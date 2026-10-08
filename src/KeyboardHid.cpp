#include "KeyboardHid.h"
#include "KeyboardLedValue.h"
#include "Diagnostics.h"
#include <Arduino.h>

namespace {
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
    uint8_t level;
    if (batteryLevel(batterySample_, lastBatteryPoll_, level)) battery_->setValue(&level, 1);
  }
  input_->setValue(reinterpret_cast<const uint8_t*>(&report_), sizeof(report_));
  const uint8_t noLeds = 0;
  output_->setValue(&noLeds, 1);
  if (!server->start()) return false; // Starts all services in NimBLE2.
  if (!batterySource_) {
    // removeService needs an actually registered handle. Rebuild only before advertising,
    // so the automatic BAS is absent even to clients with cached attribute handles.
    server->removeService(hid_->getBatteryService());
    if (!hid_->getBatteryService()->getRemoved() || !server->start()) return false;
  }
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->enableScanResponse(false);
  if (!adv->setAppearance(HID_KEYBOARD) || !adv->addServiceUUID(hid_->getHidService()->getUUID()) ||
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

int KeyboardHid::batteryReadGuard(NimBLEConnInfo&, void* arg) {
  auto* self = static_cast<KeyboardHid*>(arg);
  portENTER_CRITICAL(&self->linkMux_);
  const BatterySample sample = self->batterySample_;
  portEXIT_CRITICAL(&self->linkMux_);
  uint8_t level;
  if (!batteryLevel(sample, static_cast<uint32_t>(millis()), level)) return BLE_ATT_ERR_UNLIKELY;
  // Cache only: no backend access, measurement, retry or Serial in the callback.
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
  if (chr != output_) return;
  uint8_t value = 0;
  const bool valid = readKeyboardLed(chr, value);
  diagnostics::ledOutput(info.getConnHandle(), valid, value);
}
