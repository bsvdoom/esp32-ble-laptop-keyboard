#pragma once
#include "BatteryAdcMath.h"

#ifdef KEYBOARD_ADC_TEST_CONFIGURATION
#include "../tools/adc_test_configuration.h"
#else
namespace batteryAdcConfig {
// Enable only with explicit, reviewed physical hardware parameters below.
constexpr bool enabled = false;
inline batteryAdc::Config configuration() {
  batteryAdc::Config c;
  c.gpio = -1; // GPIO0 is the sole code-free ADC1 candidate, not verified wiring.
  c.attenuation = -1; // IDF indices: 0=0dB, 1=2.5dB, 2=6dB, 3=12dB.
  c.rTopOhm = 0;    // Battery+ -> ADC node.
  c.rBottomOhm = 0; // ADC node -> common GND.
  c.cells = 1; // User-confirmed LiPo 1S2P; parallel=2 does not change voltage.
  c.maxBatteryMv = 0; // Maximum PACK voltage, including charger tolerances.
  c.emptyCellMv = 3200; // User voltage model only; no cutoff/protection policy.
  c.fullCellMv = 4200;  // Model endpoint, not proof of physical charger maximum.
  c.minPlausibleCellMv = 0; // Below this is a measurement fault, not 0%.
  c.resistorTolerancePermille = UINT16_MAX; // UNKNOWN; rejects enabling until specified.
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
