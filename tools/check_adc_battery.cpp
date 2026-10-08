#define KEYBOARD_ADC_TEST_CONFIGURATION
#include "BatteryAdcConfig.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main() {
  using namespace batteryAdc;
  const Config c = batteryAdcConfig::configuration();
  assert(validConfig(c));
  assert(keyboardPins::freeAdc1(0));
  for (int pin = 1; pin <= 5; ++pin) { auto bad = c; bad.gpio = pin; assert(!validConfig(bad)); }
  auto bad = c; bad.gpio = -1; assert(!validConfig(bad));
  bad = c; bad.gpio = 21; assert(!validConfig(bad));
  assert(!validConfig(Config{}));
  bad = c; bad.attenuation = 4; assert(!validConfig(bad));
  bad = c; bad.attenuation = 2; assert(!validConfig(bad)); // Max ADC 2125mV >1300mV.
  bad = c; bad.rBottomOhm = 0; assert(!validConfig(bad));
  bad = c; bad.rTopOhm = 0; assert(!validConfig(bad));
  bad = c; bad.rTopOhm = UINT32_MAX; assert(!validConfig(bad));
  bad = c; bad.rTopOhm = 80000; assert(!validConfig(bad)); // Inadequate 10% headroom.
  bad = c; bad.resistorTolerancePermille = 100; assert(!validConfig(bad));
  bad = c; bad.cells = 0; assert(!validConfig(bad));
  bad = c; bad.cells = 2; assert(!validConfig(bad)); // Pack max must match cell count.
  bad = c; bad.maxBatteryMv = 5000; assert(!validConfig(bad));
  bad = c; bad.fullCellMv = bad.emptyCellMv; assert(!validConfig(bad));
  bad = c; bad.minPlausibleCellMv = bad.emptyCellMv; assert(!validConfig(bad));
  bad = c; bad.periodMs = 60000; assert(!validConfig(bad));
  bad = c; bad.samples = 0; assert(!validConfig(bad));
  bad = c; bad.samples = 17; assert(!validConfig(bad));
  bad = c; bad.gainPpm = 0; assert(!validConfig(bad));
  bad = c; bad.offsetMv = 201; assert(!validConfig(bad));

  uint32_t mv = 123;
  assert(batteryMillivolts(c, 1875, mv) && mv == 3750);
  assert(batteryMillivolts(c, 2100, mv) && mv == 4200);
  assert(!batteryMillivolts(c, 0, mv));
  assert(!batteryMillivolts(c, 1399, mv)); // 2798mV below plausible min.
  assert(!batteryMillivolts(c, 2126, mv)); // 4252mV above specified max.
  assert(!batteryMillivolts(c, UINT32_MAX, mv));
  auto corrected = c; corrected.gainPpm = 1020000; corrected.offsetMv = -10;
  assert(batteryMillivolts(corrected, 1875, mv) && mv == 3806); // round(1912.5)-10, doubled.
  uint8_t percent = 99;
  assert(voltagePercent(c, 2800, percent) && percent == 0);
  assert(voltagePercent(c, 3199, percent) && percent == 0);
  assert(voltagePercent(c, 3200, percent) && percent == 0);
  assert(voltagePercent(c, 3700, percent) && percent == 50);
  assert(voltagePercent(c, 3750, percent) && percent == 55);
  assert(voltagePercent(c, 4200, percent) && percent == 100);
  assert(voltagePercent(c, 4250, percent) && percent == 100);
  assert(!voltagePercent(c, 2799, percent));
  assert(!voltagePercent(c, 4251, percent));
  int previous = -1;
  for (uint32_t v = 2800; v <= 4250; ++v) {
    assert(voltagePercent(c, v, percent)); assert(percent >= previous && percent <= 100);
    previous = percent;
  }
  // Two cells are mathematical support, not a claim about connected hardware.
  auto two = c; two.cells = 2; two.maxBatteryMv = 8500; two.rTopOhm = 300000;
  assert(validConfig(two));
  assert(batteryMillivolts(two, 1875, mv) && mv == 7500);
  assert(voltagePercent(two, 7500, percent) && percent == 55);
  for (int raw : {-1, 0, 16, 4079, 4095, 65535}) assert(!validReading(c, raw, 1875, mv));
  assert(validReading(c, 2500, 1875, mv));
  Filter f(c);
  assert(!f.cachedSample().valid);
  assert(f.push(2500, 1860, 0) == Filter::Result::Pending);
  assert(f.push(2500, 1870, 25) == Filter::Result::Pending);
  assert(f.push(2500, 1880, 50) == Filter::Result::Pending);
  assert(f.push(2500, 1890, 75) == Filter::Result::Valid);
  assert(f.batteryMv() == 3750 && f.cachedSample().percent == 55);
  assert(f.cachedSample().valid && f.cachedSample().measuredMs == 75);
  assert(f.push(-1, 1875, 100) == Filter::Result::Invalid && !f.cachedSample().valid);
  assert(f.push(2500, 1875, 125) == Filter::Result::Pending && !f.cachedSample().valid);
  assert(f.push(2500, 2000, 150) == Filter::Result::Invalid && !f.cachedSample().valid);
  assert(f.push(2500, 1875, 200) == Filter::Result::Pending);
  assert(f.push(2500, 1875, 1201) == Filter::Result::Invalid && !f.cachedSample().valid);
  Filter wrap(c);
  assert(wrap.push(2500, 1875, 0xFFFFFFF0) == Filter::Result::Pending);
  assert(wrap.push(2500, 1875, 0x09) == Filter::Result::Pending);
  assert(wrap.push(2500, 1875, 0x22) == Filter::Result::Pending);
  assert(wrap.push(2500, 1875, 0x3B) == Filter::Result::Valid);
  assert(wrap.cachedSample().percent == 55 && wrap.cachedSample().measuredMs == 0x3B);
  assert(wrap.push(4095, 1875, 100) == Filter::Result::Invalid && !wrap.cachedSample().valid);
  std::puts("PASS ADC: divider/config/range/model/monotonicity/invalid/filter/timestamp/wrap");
}
