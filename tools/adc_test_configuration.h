#pragma once
#ifndef KEYBOARD_ADC_TEST_CONFIGURATION
#error "ADC TEST CONFIGURATION must never silently become normal hardware configuration"
#endif
// TEST CONFIGURATION: user 1S2P voltage model; hypothetical divider/maximum/minimum.
// Exercises the real ADC driver, NOT a synthetic percentage source.
namespace batteryAdcConfig {
constexpr bool enabled = true;
inline batteryAdc::Config configuration() {
  batteryAdc::Config c;
  c.gpio = 0; c.attenuation = 3;
  c.rTopOhm = 100000; c.rBottomOhm = 100000;
  c.cells = 1; c.maxBatteryMv = 4250;
  c.emptyCellMv = 3200; c.fullCellMv = 4200; c.minPlausibleCellMv = 2800;
  c.periodMs = 10000; c.sampleSpacingMs = 25; c.samples = 4;
  return c;
}
}
