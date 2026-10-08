#pragma once
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <freertos/FreeRTOS.h>
#include "KeyboardReport.h"
#include "KeyboardLink.h"
#include "BatteryState.h"

// One BLE host at a time (pinned CONFIG_BT_NIMBLE_MAX_CONNECTIONS=1).
// Report writes happen only in loop; callbacks only change the protected link state.
class KeyboardHid final : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
public:
  using SendResult = KeyboardSendResult;
  bool begin(BatterySource* batterySource = nullptr);
  // Loop-only; polls a cached backend and attempts at most one low-rate notification.
  void pollBattery(uint32_t nowMs);
  KeyboardLinkSnapshot linkSnapshot();
  // Loop-only: clear the cached readable value when a new generation is observed.
  // No notification/acceptance is implied; mandatory empty notify is separate.
  KeyboardLinkSnapshot prepareSession();
  // One loop-owned attempt, checked against the caller's session before/after notify.
  // Result describes the local API, never host receipt. No retry inside the adapter.
  SendResult submitReport(const KeyboardReport& report, uint64_t expectedGeneration);

private:
  portMUX_TYPE linkMux_ = portMUX_INITIALIZER_UNLOCKED;
  KeyboardLinkState link_;
  KeyboardReport report_;
  uint64_t attributeGeneration_ = 0;
  NimBLEHIDDevice* hid_ = nullptr; // One boot-lifetime allocation; no end/reset path.
  NimBLECharacteristic* input_ = nullptr;
  NimBLECharacteristic* output_ = nullptr;
  NimBLECharacteristic* battery_ = nullptr;
  BatterySource* batterySource_ = nullptr;
  BatteryLink batteryLink_; // Protected by linkMux_, independent of keyboard readiness.
  BatterySample batterySample_; // Protected cached copy for the read guard.
  BatteryState batteryState_; // Loop-owned; never touched in callbacks.
  uint32_t lastBatteryPoll_ = 0;
  static int batteryReadGuard(NimBLEConnInfo&, void*);
  BatteryLink batteryLinkSnapshot();

  void onConnect(NimBLEServer*, NimBLEConnInfo&) override;
  void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason) override;
  void onAuthenticationComplete(NimBLEConnInfo&) override;
  void onSubscribe(NimBLECharacteristic*, NimBLEConnInfo&, uint16_t) override;
  void onStatus(NimBLECharacteristic*, NimBLEConnInfo&, int code) override;
  void onWrite(NimBLECharacteristic*, NimBLEConnInfo&) override;
};
