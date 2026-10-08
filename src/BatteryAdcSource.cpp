#include "BatteryAdcSource.h"
#include <Arduino.h>
#include <esp_efuse_rtc_calib.h>

#if !CONFIG_IDF_TARGET_ESP32C3
#error "Battery ADC mapping/calibration is specific to the pinned ESP32-C3 baseline"
#endif

bool BatteryAdcSource::begin() {
  if (!batteryAdc::validConfig(config_)) { status_ = Status::InvalidConfig; return false; }
  channel_ = static_cast<adc1_channel_t>(config_.gpio); // C3 GPIO0..4 = ADC1 channels0..4.
  gpio_num_t actualPin;
  if (adc1_pad_get_io_num(channel_, &actualPin) != ESP_OK || actualPin != config_.gpio ||
      adc1_config_width(ADC_WIDTH_BIT_12) != ESP_OK ||
      adc1_config_channel_atten(channel_, static_cast<adc_atten_t>(config_.attenuation)) != ESP_OK) {
    status_ = Status::DriverError; return false;
  }
  // C3's pinned calibration implementation supports eFuse TP, not default Vref fallback.
  if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) != ESP_OK) {
    status_ = Status::CalibrationMissing; return false;
  }
  // Check the data before characterize(), whose C3 implementation can abort on bad data.
  uint32_t digi = 0, voltage = 0;
  if (esp_efuse_rtc_calib_get_cal_voltage(1, config_.attenuation, &digi, &voltage) != ESP_OK ||
      !digi || !voltage) { status_ = Status::CalibrationError; return false; }
  const auto calibration = esp_adc_cal_characterize(ADC_UNIT_1,
    static_cast<adc_atten_t>(config_.attenuation), ADC_WIDTH_BIT_12, 0, &characteristics_);
  if (calibration != ESP_ADC_CAL_VAL_EFUSE_TP) { status_ = Status::CalibrationError; return false; }
  initialized_ = true; status_ = Status::Ready;
  return true;
}

void BatteryAdcSource::poll(uint32_t nowMs) {
  if (!initialized_) return; // No repeated initialization or calibration retry.
  if (!burst_) {
    if (attempted_ && static_cast<uint32_t>(nowMs - lastBurstMs_) < config_.periodMs) return;
    attempted_ = true; burst_ = true; lastBurstMs_ = nowMs;
  } else if (static_cast<uint32_t>(nowMs - lastSampleMs_) < config_.sampleSpacingMs) return;
  const int raw = adc1_get_raw(channel_); // int preserves negative driver error.
  const uint32_t measuredMs = static_cast<uint32_t>(millis());
  lastSampleMs_ = measuredMs;
  const uint32_t mv = raw >= 0 && raw <= 4095 ?
    esp_adc_cal_raw_to_voltage(static_cast<uint32_t>(raw), &characteristics_) : 0;
  const auto result = filter_.push(raw, mv, measuredMs);
  if (result != batteryAdc::Filter::Result::Pending) {
    burst_ = false;
    status_ = result == batteryAdc::Filter::Result::Valid ? Status::Ready : Status::InvalidSample;
  }
}
