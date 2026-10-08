#include "BatteryState.h"

void BatteryState::update(const BatterySample& sample, const BatteryLink& link, uint32_t) {
  if (link.generation != link_.generation) acceptedValid_ = false;
  sample_ = sample;
  link_ = link;
}

bool BatteryState::nextAttempt(uint32_t now, Request& request) {
  uint8_t value;
  if (!link_.ready() || !batteryLevel(sample_, now, value) ||
      (acceptedValid_ && accepted_ == value) ||
      (attempted_ && static_cast<uint32_t>(now - lastAttempt_) < batteryConfig::notifyIntervalMs)) return false;
  request = Request{link_.generation, link_.handle, value};
  attempted_ = true;
  lastAttempt_ = now;
  return true;
}

void BatteryState::complete(const Request& request, bool localAccepted, const BatteryLink& after) {
  if (localAccepted && after.ready() && after.generation == request.generation &&
      after.handle == request.handle && link_.generation == request.generation) {
    accepted_ = request.percent;
    acceptedValid_ = true;
  }
}
