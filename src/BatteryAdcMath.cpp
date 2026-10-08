#include "BatteryAdcMath.h"

namespace batteryAdc {
uint32_t adcRangeMv(int attenuation) {
  switch (attenuation) {
    case 0: return 750;
    case 1: return 1050;
    case 2: return 1300;
    case 3: return 2500; // Pinned IDF ADC_ATTEN_DB_12 (old alias: DB_11).
    default: return 0;
  }
}

bool validConfig(const Config& c) {
  if (!keyboardPins::freeAdc1(c.gpio) || !adcRangeMv(c.attenuation) ||
      !c.rTopOhm || !c.rBottomOhm || c.rTopOhm > 10000000 || c.rBottomOhm > 10000000 ||
      !c.cells || c.cells > 4 || c.maxBatteryMv < c.cells * 4000u ||
      c.maxBatteryMv > c.cells * 4350u || c.minPlausibleCellMv < 2000 ||
      c.minPlausibleCellMv >= c.emptyCellMv || c.emptyCellMv >= c.fullCellMv ||
      c.fullCellMv * c.cells > c.maxBatteryMv || c.resistorTolerancePermille > 100 ||
      c.headroomPercent < 10 || c.headroomPercent > 50 ||
      c.periodMs < 1000 || c.periodMs > batteryConfig::maxAgeMs / 2 ||
      c.sampleSpacingMs < 10 || c.sampleSpacingMs > 1000 ||
      !c.samples || c.samples > 16 || c.samples * c.sampleSpacingMs >= c.periodMs ||
      c.maxBurstMs < c.samples * c.sampleSpacingMs || c.maxBurstMs >= c.periodMs ||
      !c.rawRailMargin || c.rawRailMargin > 128 ||
      !c.maxSpreadCellMv || c.maxSpreadCellMv > 500 ||
      c.gainPpm < 500000 || c.gainPpm > 1500000 || c.offsetMv < -200 || c.offsetMv > 200) return false;
  // Worst ratio: bottom high, top low, then require at least 10% ADC headroom.
  const uint64_t top = uint64_t(c.rTopOhm) * (1000 - c.resistorTolerancePermille);
  const uint64_t bottom = uint64_t(c.rBottomOhm) * (1000 + c.resistorTolerancePermille);
  return uint64_t(c.maxBatteryMv) * bottom * 100 <=
         uint64_t(adcRangeMv(c.attenuation)) * (100 - c.headroomPercent) * (top + bottom);
}

bool batteryMillivolts(const Config& c, uint32_t adcMv, uint32_t& batteryMv) {
  if (!validConfig(c) || adcMv > adcRangeMv(c.attenuation)) return false;
  const int64_t corrected = (int64_t(adcMv) * c.gainPpm + 500000) / 1000000 + c.offsetMv;
  if (corrected <= 0 || corrected > adcRangeMv(c.attenuation)) return false;
  const uint64_t numerator = uint64_t(corrected) * (uint64_t(c.rTopOhm) + c.rBottomOhm);
  const uint64_t value = (numerator + c.rBottomOhm / 2) / c.rBottomOhm;
  if (value < c.minPlausibleCellMv * c.cells || value > c.maxBatteryMv) return false;
  batteryMv = static_cast<uint32_t>(value);
  return true;
}

bool voltagePercent(const Config& c, uint32_t mv, uint8_t& percent) {
  if (!validConfig(c) || mv < c.minPlausibleCellMv * c.cells || mv > c.maxBatteryMv) return false;
  const uint32_t empty = c.emptyCellMv * c.cells, full = c.fullCellMv * c.cells;
  percent = mv <= empty ? 0 : mv >= full ? 100 :
    static_cast<uint8_t>(((mv - empty) * 100 + (full - empty) / 2) / (full - empty));
  return true; // Voltage estimate only, not measured capacity or protection.
}

bool validReading(const Config& c, int raw, uint32_t adcMv, uint32_t& batteryMv) {
  return validConfig(c) && raw > c.rawRailMargin && raw < 4095 - c.rawRailMargin &&
         batteryMillivolts(c, adcMv, batteryMv);
}

Filter::Result Filter::push(int raw, uint32_t adcMv, uint32_t measuredMs) {
  uint32_t mv;
  if (!validReading(config_, raw, adcMv, mv) ||
      (count_ && static_cast<uint32_t>(measuredMs - firstMs_) > config_.maxBurstMs)) {
    count_ = 0; sum_ = 0; sample_.valid = false;
    return Result::Invalid;
  }
  if (!count_) { min_ = max_ = mv; sum_ = 0; firstMs_ = measuredMs; }
  if (mv < min_) min_ = mv;
  if (mv > max_) max_ = mv;
  if (max_ - min_ > config_.maxSpreadCellMv * config_.cells) {
    count_ = 0; sum_ = 0; sample_.valid = false;
    return Result::Invalid;
  }
  sum_ += mv;
  if (++count_ < config_.samples) return Result::Pending;
  lastBatteryMv_ = (sum_ + count_ / 2) / count_;
  count_ = 0;
  uint8_t percent;
  sample_.valid = voltagePercent(config_, lastBatteryMv_, percent);
  if (!sample_.valid) return Result::Invalid;
  sample_.percent = percent;
  sample_.measuredMs = measuredMs; // Last actual conversion of the accepted burst.
  return Result::Valid;
}
}
