#pragma once
#include "KeyboardConfig.h"
#include "KeyboardLink.h"
#include "KeyboardState.h"

// Loop-owned, fixed storage. No BLE/Arduino calls or callback writes.
class KeyboardSession {
public:
  struct Request {
    uint64_t generation = 0, token = 0;
    KeyboardReport report;
    bool initialEmpty = false;
  };
  struct Metrics {
    KeyboardLinkSnapshot link;
    uint32_t attempts = 0, retries = 0, rejected = 0, overflows = 0, discarded = 0, stale = 0;
    uint32_t wantedChanges = 0;
    uint16_t suppressed = 0;
    uint8_t pending = 0;
    bool initialEmpty = false, localValid = false;
  };
  void update(const KeyboardLinkSnapshot&, const KeyboardState&, const UsageMatrix&, uint32_t nowMs);
  bool nextAttempt(uint32_t nowMs, Request&);
  void complete(const Request&, KeyboardSendResult, const KeyboardLinkSnapshot& afterSend);
  const KeyboardReport& wantedReport() const { return wanted_; }
  const KeyboardReport& locallyAcceptedReport() const { return accepted_; }
  bool isSuppressed(unsigned row, unsigned col) const { return suppressed_[row][col]; }
  Metrics metrics() const;

private:
  KeyboardLinkSnapshot link_;
  PhysicalMatrix suppressed_ = {};
  KeyboardReport wanted_, accepted_;
  KeyboardReport queue_[keyboardConfig::reportQueueCapacity];
  uint8_t head_ = 0, count_ = 0;
  bool initialized_ = false, initialEmpty_ = false, localValid_ = false;
  bool attempted_ = false, inFlight_ = false, blocked_ = false, headFailed_ = false;
  uint32_t lastAttempt_ = 0, waitMs_ = keyboardConfig::sendIntervalMs;
  uint64_t token_ = 0;
  Metrics totals_;
  void enqueue(const KeyboardReport&);
};
