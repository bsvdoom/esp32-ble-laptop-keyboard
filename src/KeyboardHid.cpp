#include "KeyboardHid.h"
#include "KeyboardLedValue.h"
#include "Diagnostics.h"
#include <Arduino.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_timer.h>

using keyboardble::OtaBackend;
using keyboardble::OtaError;
using keyboardble::OtaSession;
using keyboardble::OtaState;
using keyboardble::OtaStatus;

namespace {
class Esp32OtaBackend final : public OtaBackend {
public:
    uint32_t maxFirmwareSize() const override {
        const esp_partition_t *part = esp_ota_get_next_update_partition(nullptr);
        return part ? part->size : 0;
    }
    bool begin(uint32_t size) override {
        abort();
        _partition = esp_ota_get_next_update_partition(nullptr);
        if (!_partition || size == 0 || size > _partition->size) return false;
        if (esp_ota_begin(_partition, OTA_WITH_SEQUENTIAL_WRITES, &_handle) != ESP_OK) {
            _partition = nullptr;
            return false;
        }
        _active = true;
        return true;
    }
    bool write(const uint8_t *data, size_t length) override {
        return _active && esp_ota_write(_handle, data, length) == ESP_OK;
    }
    bool end() override {
        if (!_active || !_partition) return false;
        _active = false;
        if (esp_ota_end(_handle) != ESP_OK) return false;
        return esp_ota_set_boot_partition(_partition) == ESP_OK;
    }
    void abort() override {
        if (_active) {
            esp_ota_abort(_handle);
            _active = false;
            _partition = nullptr;
        }
    }
private:
    const esp_partition_t *_partition = nullptr;
    esp_ota_handle_t _handle = 0;
    bool _active = false;
};
Esp32OtaBackend otaBackend;

// Scoped FreeRTOS mutex; tolerates a null handle before begin().
class OtaLock {
public:
  explicit OtaLock(SemaphoreHandle_t mutex) : mutex_(mutex) {
    if (mutex_) xSemaphoreTake(mutex_, portMAX_DELAY);
  }
  ~OtaLock() { if (mutex_) xSemaphoreGive(mutex_); }
  OtaLock(const OtaLock&) = delete;
  OtaLock& operator=(const OtaLock&) = delete;
private:
  SemaphoreHandle_t mutex_;
};

// Independently specified using USB HID 1.11 items and Usage Tables page 0x07.
// ID 1: 8 modifier bits, reserved byte, six usage bytes; output: five LED bits.
// No consumer report: none of the physical matrix cells uses it.
uint8_t reportMap[] = {
  0x05, 0x01,       // Usage Page (Generic Desktop)
  0x09, 0x06,       // Usage (Keyboard)
  0xa1, 0x01,       // Collection (Application)
  0x85, 0x01,       // Report ID 1
  0x05, 0x07,       // Usage Page (Keyboard/Keypad)
  0x19, 0xe0, 0x29, 0xe7, // Modifier usages
  0x15, 0x00, 0x25, 0x01, // Logical 0..1
  0x75, 0x01, 0x95, 0x08, // Eight one-bit fields
  0x81, 0x02,       // Input (Data, Variable, Absolute)
  0x75, 0x08, 0x95, 0x01,
  0x81, 0x01,       // Reserved byte (Constant)
  0x05, 0x08,       // Usage Page (LED)
  0x19, 0x01, 0x29, 0x05,
  0x75, 0x01, 0x95, 0x05,
  0x91, 0x02,       // Output: five LED bits
  0x75, 0x03, 0x95, 0x01,
  0x91, 0x01,       // Output padding
  0x05, 0x07,
  0x19, 0x00, 0x29, 0x65, // Normal key usages (modifiers separate)
  0x15, 0x00, 0x25, 0x65,
  0x75, 0x08, 0x95, 0x06,
  0x81, 0x00,       // Input (Data, Array, Absolute)
  0xc0
};

int observeGap(ble_gap_event* event, void*) {
  if (event->type == BLE_GAP_EVENT_ENC_CHANGE)
    diagnostics::encryption(event->enc_change.conn_handle, event->enc_change.status);
  else if (event->type == BLE_GAP_EVENT_ADV_COMPLETE)
    diagnostics::advertisingComplete(event->adv_complete.reason);
  return 0; // Passive listener; does not modify stack policy.
}
}

bool KeyboardHid::begin(BatterySource* batterySource) {
  if (!NimBLEDevice::init("ESP32 Keyboard")) return false;
  // Keep the old bonding/MITM/Secure Connections request and default IO capability.
  // No automatic bond deletion or boot-time NVS migration in application code.
  NimBLEDevice::setSecurityAuth(true, true, true);
  if (!NimBLEDevice::setCustomGapHandler(observeGap)) return false;
  NimBLEServer* server = NimBLEDevice::createServer();
  if (!server) return false;
  server->setCallbacks(this, false); // Global object, never delete as stack-owned callback.
  server->advertiseOnDisconnect(true); // NimBLE2 defaults to false.
  hid_ = new NimBLEHIDDevice(server);
  input_ = hid_->getInputReport(1);
  output_ = hid_->getOutputReport(1);
  if (!input_ || !output_ || !hid_->setManufacturer("Espressif Systems")) return false;
  input_->setCallbacks(this);
  output_->setCallbacks(this);
  // Historical baseline identity values; not a claim of assigned vendor ownership.
  hid_->setPnp(0x02, 0x05ac, 0x820a, 0x0210);
  hid_->setHidInfo(0x00, 0x01);
  hid_->setReportMap(reportMap, sizeof(reportMap));
  batterySource_ = batterySource;
  if (batterySource_) {
    battery_ = hid_->getBatteryLevel();
    if (!battery_) return false;
    battery_->setReadGuard(batteryReadGuard, this);
    battery_->setCallbacks(this);
    batterySample_ = batterySource_->cachedSample();
    lastBatteryPoll_ = static_cast<uint32_t>(millis());
    uint8_t level = 50;
    if (batteryLevel(batterySample_, lastBatteryPoll_, level)) {
      battery_->setValue(&level, 1);
    } else {
      battery_->setValue(&level, 1);
    }
  }
  input_->setValue(reinterpret_cast<const uint8_t*>(&report_), sizeof(report_));
  const uint8_t noLeds = 0;
  output_->setValue(&noLeds, 1);
  { // Simple BLE OTA service (kodira/simple-ble-ota compatible, encrypted + bonded only).
    otaMutex_ = xSemaphoreCreateMutexStatic(&otaMutexStorage_);
    auto* ota = server->createService(NimBLEUUID(OtaSession::ServiceUuid));
    if (!otaMutex_ || !ota) return false;
    otaControl_ = ota->createCharacteristic(NimBLEUUID(OtaSession::ControlUuid),
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::NOTIFY, 16);
    otaData_ = ota->createCharacteristic(NimBLEUUID(OtaSession::DataUuid),
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::WRITE_ENC, 512);
    if (!otaControl_ || !otaData_) return false;
    otaControl_->setCallbacks(this);
    otaData_->setCallbacks(this);
  }
  if (!server->start()) return false; // Starts all services in NimBLE2.
  if (!batterySource_) {
    // removeService needs an actually registered handle. Rebuild only before advertising,
    // so the automatic BAS is absent even to clients with cached attribute handles.
    server->removeService(hid_->getBatteryService());
    if (!hid_->getBatteryService()->getRemoved() || !server->start()) return false;
  }
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->enableScanResponse(true);
  if (!adv->setAppearance(HID_KEYBOARD) || !adv->addServiceUUID(hid_->getHidService()->getUUID()) ||
      (batterySource_ && !adv->addServiceUUID(hid_->getBatteryService()->getUUID())) ||
      !adv->setName("ESP32 Keyboard")) return false;
  const bool started = adv->start();
  diagnostics::advertisingStart(started);
  return started;
}

BatteryLink KeyboardHid::batteryLinkSnapshot() {
  portENTER_CRITICAL(&linkMux_);
  const BatteryLink copy = batteryLink_;
  portEXIT_CRITICAL(&linkMux_);
  return copy;
}

int KeyboardHid::batteryReadGuard(NimBLEConnInfo& info, void* arg) {
  auto* self = static_cast<KeyboardHid*>(arg);
  portENTER_CRITICAL(&self->linkMux_);
  const BatterySample sample = self->batterySample_;
  portEXIT_CRITICAL(&self->linkMux_);
  uint8_t level = 0;
  const bool ok = batteryLevel(sample, static_cast<uint32_t>(millis()), level);
  Serial.printf("BATTERY_READ handle=%u ok=%d lvl=%u\n", info.getConnHandle(), ok, level);
  if (!ok) return BLE_ATT_ERR_UNLIKELY;
  self->battery_->setValue(&level, 1);
  return 0;
}

void KeyboardHid::pollBattery(uint32_t nowMs) {
  if (!batterySource_) return;
  if (static_cast<uint32_t>(nowMs - lastBatteryPoll_) >= batteryConfig::pollMs) {
    const BatterySample sample = batterySource_->cachedSample();
    portENTER_CRITICAL(&linkMux_);
    batterySample_ = sample;
    portEXIT_CRITICAL(&linkMux_);
    lastBatteryPoll_ = nowMs;
  }
  portENTER_CRITICAL(&linkMux_);
  const BatterySample sample = batterySample_;
  const BatteryLink before = batteryLink_;
  portEXIT_CRITICAL(&linkMux_);
  uint8_t level;
  if (batteryLevel(sample, nowMs, level)) battery_->setValue(&level, 1);
  batteryState_.update(sample, before, nowMs);
  BatteryState::Request request;
  if (!batteryState_.nextAttempt(nowMs, request)) return;
  const auto check = batteryLinkSnapshot();
  if (!check.ready() || check.generation != request.generation) return;
  const bool accepted = battery_->notify(&request.percent, 1, request.handle);
  Serial.printf("BATTERY_NOTIFY handle=%u pct=%u accepted=%d\n", request.handle, request.percent, accepted);
  batteryState_.complete(request, accepted, batteryLinkSnapshot());
  // Bool means local stack acceptance. No keyboard counters/FIFO/session are changed.
}

KeyboardLinkSnapshot KeyboardHid::linkSnapshot() {
  portENTER_CRITICAL(&linkMux_);
  const KeyboardLinkSnapshot copy = link_.snapshot();
  portEXIT_CRITICAL(&linkMux_);
  return copy;
}

KeyboardLinkSnapshot KeyboardHid::prepareSession() {
  const auto link = linkSnapshot();
  if (attributeGeneration_ != link.generation) {
    report_ = KeyboardReport{};
    if (input_) input_->setValue(reinterpret_cast<const uint8_t*>(&report_), sizeof(report_));
    attributeGeneration_ = link.generation;
  }
  return link;
}

KeyboardHid::SendResult KeyboardHid::submitReport(const KeyboardReport& report, uint64_t expectedGeneration) {
  const KeyboardLinkSnapshot before = linkSnapshot();
  if (before.generation != expectedGeneration) return SendResult::StaleSession;
  if (!before.connected) {
    diagnostics::reportSkipped(false);
    return SendResult::Disconnected;
  }
  if (!before.ready() || !input_) {
    diagnostics::reportSkipped(true);
    return SendResult::NotReady;
  }
  report_ = report;
  report_.reserved = 0;
  input_->setValue(reinterpret_cast<const uint8_t*>(&report_), sizeof(report_));
  // Short snapshot check only: no mutex spans GATT or notify.
  if (linkSnapshot().generation != expectedGeneration) return SendResult::StaleSession;
  diagnostics::reportAttempt();
  const bool accepted = input_->notify(reinterpret_cast<const uint8_t*>(&report_), sizeof(report_), before.handle);
  diagnostics::notifyResult(accepted);
  // A check/notify race is not eliminated by the handle-only stack API.
  // Do not credit a result to a session that changed during the call.
  const auto after = linkSnapshot();
  if (after.generation != expectedGeneration) return SendResult::StaleSession;
  if (!after.connected) return SendResult::Disconnected;
  if (!after.ready()) return SendResult::NotReady;
  return accepted ? SendResult::LocalAccepted : SendResult::LocalRejected;
}

void KeyboardHid::onConnect(NimBLEServer*, NimBLEConnInfo& info) {
  const uint16_t handle = info.getConnHandle();
  const bool encrypted = info.isEncrypted(), authenticated = info.isAuthenticated(), bonded = info.isBonded();
  portENTER_CRITICAL(&linkMux_);
  link_.connect(handle, encrypted, authenticated, bonded);
  batteryLink_.connect(handle, encrypted);
  portEXIT_CRITICAL(&linkMux_);
  diagnostics::connect(handle);
}

void KeyboardHid::onDisconnect(NimBLEServer*, NimBLEConnInfo& info, int reason) {
  const uint16_t handle = info.getConnHandle();
  portENTER_CRITICAL(&linkMux_);
  link_.disconnect(handle);
  batteryLink_.disconnect(handle);
  portEXIT_CRITICAL(&linkMux_);
  diagnostics::disconnect(handle, reason);
  // Only scalar link state here; suppression and report queue belong to loop.
  OtaLock lock(otaMutex_);
  otaSession_.onDisconnect(otaBackend); // Mid-upload: esp_ota_abort, old firmware stays.
}

void KeyboardHid::onAuthenticationComplete(NimBLEConnInfo& info) {
  const uint16_t handle = info.getConnHandle();
  const bool encrypted = info.isEncrypted(), authenticated = info.isAuthenticated(), bonded = info.isBonded();
  portENTER_CRITICAL(&linkMux_);
  link_.security(handle, encrypted, authenticated, bonded);
  batteryLink_.security(handle, encrypted);
  portEXIT_CRITICAL(&linkMux_);
  diagnostics::authentication(handle, encrypted, authenticated, bonded);
}

void KeyboardHid::onSubscribe(NimBLECharacteristic* chr, NimBLEConnInfo& info, uint16_t value) {
  if (battery_ && chr == battery_) {
    portENTER_CRITICAL(&linkMux_);
    batteryLink_.security(info.getConnHandle(), info.isEncrypted());
    batteryLink_.subscription(info.getConnHandle(), (value & 1) != 0);
    portEXIT_CRITICAL(&linkMux_);
    Serial.printf("BATTERY_SUBSCRIBE handle=%u val=%u enc=%d\n", info.getConnHandle(), value, info.isEncrypted());
    return; // Independent battery CCCD; does not change keyboard generation/readiness.
  }
  if (chr != input_) return;
  const uint16_t handle = info.getConnHandle();
  const bool encrypted = info.isEncrypted(), authenticated = info.isAuthenticated(), bonded = info.isBonded();
  portENTER_CRITICAL(&linkMux_);
  link_.subscription(handle, (value & 1) != 0, encrypted, authenticated, bonded);
  portEXIT_CRITICAL(&linkMux_);
  diagnostics::subscription(handle, value);
}

void KeyboardHid::onStatus(NimBLECharacteristic* chr, NimBLEConnInfo&, int code) {
  // Pinned NOTIFY_TX does not populate ConnInfo or identify our request/generation.
  // Aggregate diagnostics only: never complete/pop/retry a session report here.
  if (chr == input_) diagnostics::notifyStatus(code);
}

void KeyboardHid::onWrite(NimBLECharacteristic* chr, NimBLEConnInfo& info) {
  if (chr && (chr == otaControl_ || chr == otaData_)) {
    onOtaWrite(chr, info, chr == otaControl_);
    return;
  }
  if (chr != output_) return;
  uint8_t value = 0;
  const bool valid = readKeyboardLed(chr, value);
  diagnostics::ledOutput(info.getConnHandle(), valid, value);
}

// NimBLE host task. Only encrypted + bonded links with a 16-byte key may drive OTA;
// any other write is rejected and aborts a running session.
void KeyboardHid::onOtaWrite(NimBLECharacteristic* chr, NimBLEConnInfo& info, bool control) {
  const auto value = chr->getValue();
  const bool authorized = info.isEncrypted() && info.isBonded() && info.getSecKeySize() == 16;
  OtaError err = OtaError::None;
  bool notifyErr = false;
  bool requestFastConn = false;
  {
    OtaLock lock(otaMutex_);
    const OtaState prev = otaSession_.status().state;
    if (control) {
      notifyErr = otaSession_.onControlWrite(value.data(), value.size(), authorized, otaBackend, err);
    } else {
      notifyErr = otaSession_.onDataWrite(value.data(), value.size(), authorized, otaBackend, err);
    }
    const OtaState next = otaSession_.status().state;
    if (prev != OtaState::Receiving && next == OtaState::Receiving) {
      requestFastConn = true;
    } else if (next == OtaState::RebootPending && !rebootPending_) {
      rebootPending_ = true;
      rebootAtUs_ = esp_timer_get_time();
    }
  }
  if (requestFastConn) {
    NimBLEServer* server = NimBLEDevice::getServer();
    if (server) server->updateConnParams(info.getConnHandle(), 6, 12, 0, 200);
  }
  if (notifyErr && otaControl_) {
    const uint8_t frame[2] = {0xFF, static_cast<uint8_t>(err)};
    otaControl_->notify(frame, sizeof(frame), info.getConnHandle());
  }
}

OtaStatus KeyboardHid::otaStatus() {
  OtaLock lock(otaMutex_);
  return otaSession_.status();
}

void KeyboardHid::pollOta() {
  bool doReboot = false;
  {
    OtaLock lock(otaMutex_);
    doReboot = rebootPending_ && esp_timer_get_time() - rebootAtUs_ >= 250000;
  }
  if (doReboot) esp_restart();
}
