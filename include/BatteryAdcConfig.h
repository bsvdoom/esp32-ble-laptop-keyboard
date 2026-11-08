#pragma once
#include "BatteryAdcMath.h"

#ifdef KEYBOARD_ADC_TEST_CONFIGURATION
#include "../tools/adc_test_configuration.h"
#else
namespace batteryAdcConfig {
// Enable only with explicit, reviewed physical hardware parameters below.
constexpr bool enabled = true;
inline batteryAdc::Config configuration() {
  batteryAdc::Config c;
  c.gpio = 0; // GPIO0 is the sole code-free ADC1 channel on this pinout.
  c.attenuation = 3; // IDF indices: 0=0dB, 1=2.5dB, 2=6dB, 3=12dB (2500mV range).
  c.rTopOhm = 672000;    // Measured hardware divider: 3930mV bat -> 509mV ADC.
  c.rBottomOhm = 100000; // Ratio ~7.72x (0.46V-0.51V measured on GPIO0).
  c.cells = 1; // User-confirmed LiPo 1S2P; parallel=2 does not change voltage.
  c.maxBatteryMv = 4250; // Maximum PACK voltage, including charger tolerances.
  c.emptyCellMv = 3200; // User voltage model only; no cutoff/protection policy.
  c.fullCellMv = 4200;  // Model endpoint, not proof of physical charger maximum.
  c.minPlausibleCellMv = 2800; // Below this is a measurement fault, not 0%.
  c.resistorTolerancePermille = 10; // 1% resistor tolerance (10 permille).
  c.headroomPercent = 10;
  c.periodMs = 30000; // At most maxAgeMs/2; no long delays inside poll().
  c.sampleSpacingMs = 25;
  c.maxBurstMs = 1000; // Reject old partial burst after a long loop stall.
  c.samples = 4; // Arithmetic mean of valid samples in a spaced burst.
  c.rawRailMargin = 16;
  c.maxSpreadCellMv = 100; // Reject noisy burst rather than smoothing a fault.
  c.gainPpm = 1000000; // Identity, NOT measured multimeter calibration.
  c.offsetMv = 0;     // Applied to calibrated ADC mV before divider conversion.
  return c;
}
}
#endif
