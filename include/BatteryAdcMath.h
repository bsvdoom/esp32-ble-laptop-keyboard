#pragma once
#include "BatteryState.h"
#include "KeyboardPins.h"

namespace batteryAdc {
struct Config {
  int gpio = -1, attenuation = -1; // IDF enum indices 0/1/2/3, ADC1 only.
  uint32_t rTopOhm = 0, rBottomOhm = 0;
  uint8_t cells = 0;
  uint32_t maxBatteryMv = 0;
  uint16_t emptyCellMv = 0, fullCellMv = 0, minPlausibleCellMv = 0;
  uint16_t resistorTolerancePermille = 10; // Each resistor, worst-case ratio.
  uint8_t headroomPercent = 10;
  uint32_t periodMs = 30000, sampleSpacingMs = 25, maxBurstMs = 1000;
  uint8_t samples = 4, rawRailMargin = 16;
  uint16_t maxSpreadCellMv = 100;
  // Additional multimeter correction of calibrated ADC mV, not measured defaults.
  uint32_t gainPpm = 1000000;
  int32_t offsetMv = 0;
};
uint32_t adcRangeMv(int attenuation);
bool validConfig(const Config& config);
bool batteryMillivolts(const Config& config, uint32_t adcMv, uint32_t& batteryMv);
bool voltagePercent(const Config& config, uint32_t batteryMv, uint8_t& percent);
bool validReading(const Config& config, int raw, uint32_t adcMv, uint32_t& batteryMv);

// Arithmetic mean of a bounded burst. One invalid member invalidates immediately.
class Filter {
public:
  enum class Result { Pending, Valid, Invalid };
  explicit Filter(const Config& config) : config_(config) {}
  Result push(int raw, uint32_t adcMv, uint32_t measuredMs);
  BatterySample cachedSample() const { return sample_; }
  uint32_t batteryMv() const { return lastBatteryMv_; }
private:
  Config config_;
  BatterySample sample_;
  uint32_t sum_ = 0, min_ = 0, max_ = 0, lastBatteryMv_ = 0, firstMs_ = 0;
  uint8_t count_ = 0;
};
}
