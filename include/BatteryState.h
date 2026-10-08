#pragma once
#include <stdint.h>

// A backend supplies an already measured/calculated percentage, never raw ESP VCC.
struct BatterySample {
  int percent = 0;
  uint32_t measuredMs = 0;
  bool valid = false;
};

class BatterySource {
public:
  virtual ~BatterySource() = default;
  // Loop-only, bounded cached copy. Must not perform a blocking ADC/I2C measurement.
  virtual BatterySample cachedSample() = 0;
};

namespace batteryConfig {
constexpr uint32_t pollMs = 1000;
constexpr uint32_t maxAgeMs = 60000;
constexpr uint32_t notifyIntervalMs = 1000; // Also spaces retries of the latest value.
static_assert(maxAgeMs < 0x80000000u && pollMs > 0 && notifyIntervalMs > 0,
              "Battery timing must support millis wraparound");
}

inline bool batteryLevel(const BatterySample& sample, uint32_t now, uint8_t& value) {
  if (!sample.valid || sample.percent < 0 || sample.percent > 100 ||
      static_cast<uint32_t>(now - sample.measuredMs) > batteryConfig::maxAgeMs) return false;
  value = static_cast<uint8_t>(sample.percent);
  return true;
}

// Separate from the keyboard generation: keyboard CCCD changes cannot resync battery.
struct BatteryLink {
  uint64_t generation = 0;
  uint16_t handle = UINT16_MAX;
  bool connected = false, encrypted = false, subscribed = false;
  bool ready() const { return connected && encrypted && subscribed; }
  void connect(uint16_t h, bool enc) {
    ++generation; handle = h; connected = true; encrypted = enc; subscribed = false;
  }
  void disconnect(uint16_t h) {
    if (connected && handle == h) { ++generation; connected = encrypted = subscribed = false; handle = UINT16_MAX; }
  }
  void security(uint16_t h, bool enc) {
    if (connected && handle == h && encrypted != enc) { ++generation; encrypted = enc; }
  }
  void subscription(uint16_t h, bool notify) {
    if (connected && handle == h && subscribed != notify) { ++generation; subscribed = notify; }
  }
};

// Loop-owned, constant memory, latest-value coalescing. No historical battery events.
class BatteryState {
public:
  struct Request { uint64_t generation; uint16_t handle; uint8_t percent; };
  void update(const BatterySample& sample, const BatteryLink& link, uint32_t now);
  bool nextAttempt(uint32_t now, Request& request);
  void complete(const Request& request, bool localAccepted, const BatteryLink& after);
private:
  BatterySample sample_;
  BatteryLink link_;
  uint32_t lastAttempt_ = 0;
  uint8_t accepted_ = 0;
  bool acceptedValid_ = false, attempted_ = false;
};
