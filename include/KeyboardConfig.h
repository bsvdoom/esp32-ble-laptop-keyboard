#pragma once
#include <stdint.h>

namespace keyboardConfig {
// Initial value, not hardware-measured. Same defer interval for press/release.
constexpr uint32_t debounceMs = 20;
static_assert(debounceMs > 0 && debounceMs < 0x80000000u, "Invalid debounce interval");
// Initial transport policy, not measured BLE connection intervals or host ACKs.
constexpr uint8_t reportQueueCapacity = 8;
constexpr uint32_t sendIntervalMs = 20;
constexpr uint32_t retryInitialMs = 20;
constexpr uint32_t retryMaxMs = 200;
static_assert(reportQueueCapacity >= 2, "Recovery needs empty + current report");
static_assert(sendIntervalMs > 0 && retryInitialMs >= sendIntervalMs &&
              retryMaxMs >= retryInitialMs && retryMaxMs < 0x80000000u, "Invalid retry intervals");
}
