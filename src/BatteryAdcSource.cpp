#include "BatteryAdcSource.h"
#include <Arduino.h>
#include <esp_efuse_rtc_calib.h>

#if !CONFIG_IDF_TARGET_ESP32C3
#error "Battery ADC mapping/calibration is specific to the pinned ESP32-C3 baseline"
#endif

bool BatteryAdcSource::begin() {
  if (!batteryAdc::validConfig(config_)) { status_ = Status::InvalidConfig; return false; }
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  adc_unit_t unit = ADC_UNIT_1;
  int ioPin = -1;
  if (adc_oneshot_io_to_channel(config_.gpio, &unit, &channel_) != ESP_OK || unit != ADC_UNIT_1 ||
      adc_oneshot_channel_to_io(unit, channel_, &ioPin) != ESP_OK || ioPin != config_.gpio) {
    status_ = Status::DriverError; return false;
  }
  adc_oneshot_unit_init_cfg_t unitCfg = {};
  unitCfg.unit_id = ADC_UNIT_1;
  unitCfg.ulp_mode = ADC_ULP_MODE_DISABLE;
  if (adc_oneshot_new_unit(&unitCfg, &oneshot_) != ESP_OK || !oneshot_) {
    status_ = Status::DriverError; return false;
  }
  adc_oneshot_chan_cfg_t chanCfg = {};
  chanCfg.atten = static_cast<adc_atten_t>(config_.attenuation);
  chanCfg.bitwidth = ADC_BITWIDTH_12;
  if (adc_oneshot_config_channel(oneshot_, channel_, &chanCfg) != ESP_OK) {
    adc_oneshot_del_unit(oneshot_); oneshot_ = nullptr;
    status_ = Status::DriverError; return false;
  }
  uint32_t digi = 0, voltage = 0;
  if (esp_efuse_rtc_calib_get_cal_voltage(ESP_EFUSE_ADC_CALIB_VER, static_cast<uint32_t>(ADC_UNIT_1),
                                          config_.attenuation, &digi, &voltage) != ESP_OK ||
      !digi || !voltage) {
    adc_oneshot_del_unit(oneshot_); oneshot_ = nullptr;
    status_ = Status::CalibrationError; return false;
  }
  adc_cali_curve_fitting_config_t caliCfg = {};
  caliCfg.unit_id = ADC_UNIT_1;
  caliCfg.chan = channel_;
  caliCfg.atten = static_cast<adc_atten_t>(config_.attenuation);
  caliCfg.bitwidth = ADC_BITWIDTH_12;
  const esp_err_t calErr = adc_cali_create_scheme_curve_fitting(&caliCfg, &cali_);
  if (calErr != ESP_OK || !cali_) {
    adc_oneshot_del_unit(oneshot_); oneshot_ = nullptr;
    status_ = calErr == ESP_ERR_NOT_SUPPORTED ? Status::CalibrationMissing : Status::CalibrationError;
    return false;
  }
  initialized_ = true; status_ = Status::Ready;
  return true;
#else
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
#endif
}

void BatteryAdcSource::poll(uint32_t nowMs) {
  if (!initialized_) return; // No repeated initialization or calibration retry.
  if (!burst_) {
    if (attempted_ && static_cast<uint32_t>(nowMs - lastBurstMs_) < config_.periodMs) return;
    attempted_ = true; burst_ = true; lastBurstMs_ = nowMs;
  } else if (static_cast<uint32_t>(nowMs - lastSampleMs_) < config_.sampleSpacingMs) return;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  int raw = -1, calMv = 0;
  if (adc_oneshot_read(oneshot_, channel_, &raw) != ESP_OK) raw = -1;
  const uint32_t measuredMs = static_cast<uint32_t>(millis());
  lastSampleMs_ = measuredMs;
  const uint32_t mv = (raw >= 0 && raw <= 4095 &&
                       adc_cali_raw_to_voltage(cali_, raw, &calMv) == ESP_OK && calMv > 0)
                        ? static_cast<uint32_t>(calMv) : 0;
#else
  const int raw = adc1_get_raw(channel_); // int preserves negative driver error.
  const uint32_t measuredMs = static_cast<uint32_t>(millis());
  lastSampleMs_ = measuredMs;
  const uint32_t mv = raw >= 0 && raw <= 4095 ?
    esp_adc_cal_raw_to_voltage(static_cast<uint32_t>(raw), &characteristics_) : 0;
#endif
  const auto result = filter_.push(raw, mv, measuredMs);
  if (result != batteryAdc::Filter::Result::Pending) {
    burst_ = false;
    status_ = result == batteryAdc::Filter::Result::Valid ? Status::Ready : Status::InvalidSample;
    const auto sample = filter_.cachedSample();
    Serial.printf("BATTERY_BURST res=%d raw=%d adc_mv=%" PRIu32 " bat_mv=%" PRIu32 " status=%u valid=%d pct=%d\n",
                  static_cast<int>(result), raw, mv, filter_.batteryMv(),
                  static_cast<unsigned>(status_), sample.valid, sample.percent);
  }
}
