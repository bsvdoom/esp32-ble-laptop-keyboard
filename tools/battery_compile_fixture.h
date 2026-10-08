#pragma once
// Compile-check ONLY. The normal build never defines this macro or includes this file.
#ifndef KEYBOARD_BATTERY_COMPILE_CHECK
#error "Battery fixture must not enter a normal firmware"
#endif
#include "BatteryState.h"
#include <Arduino.h>
class BatteryCompileFixture final : public BatterySource {
public:
  BatterySample cachedSample() override {
    BatterySample sample;
    sample.percent = 42;
    sample.measuredMs = static_cast<uint32_t>(millis());
    sample.valid = true;
    return sample;
  }
};
static BatteryCompileFixture batteryCompileFixture;
