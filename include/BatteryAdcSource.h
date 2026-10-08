#pragma once
#include "BatteryAdcMath.h"
#include <esp_adc_cal.h>

class BatteryAdcSource : public BatterySource {
public:
  enum class Status { NotStarted, InvalidConfig, DriverError, CalibrationMissing,
                      CalibrationError, Ready, InvalidSample };
  explicit BatteryAdcSource(const batteryAdc::Config& config) : config_(config), filter_(config) {}
  bool begin(); // Loop/setup only. No eFuse writes; unavailable calibration => no data.
  void poll(uint32_t nowMs); // At most one synchronous ADC conversion, no retry loop/delay.
  BatterySample cachedSample() override { return filter_.cachedSample(); }
  Status status() const { return status_; }
  uint32_t batteryMv() const { return filter_.batteryMv(); }
private:
  batteryAdc::Config config_;
  batteryAdc::Filter filter_;
  esp_adc_cal_characteristics_t characteristics_ = {};
  adc1_channel_t channel_ = ADC1_CHANNEL_0;
  Status status_ = Status::NotStarted;
  bool initialized_ = false, burst_ = false, attempted_ = false;
  uint32_t lastBurstMs_ = 0, lastSampleMs_ = 0;
};
