#include "KeyboardSession.h"

namespace {
void increment(uint32_t& n) { if (n != UINT32_MAX) ++n; }
void add(uint32_t& n, uint8_t amount) { n += amount <= UINT32_MAX - n ? amount : UINT32_MAX - n; }
}

void KeyboardSession::enqueue(const KeyboardReport& report) {
  if (count_ && sameKeyboardReport(queue_[(head_ + count_ - 1) % keyboardConfig::reportQueueCapacity], report)) return;
  if (count_ == keyboardConfig::reportQueueCapacity) {
    increment(totals_.overflows);
    add(totals_.discarded, count_);
    head_ = count_ = 0;
    // Replace overloaded history with a release barrier and the final state.
    queue_[count_++] = KeyboardReport{};
    if (!sameKeyboardReport(report, KeyboardReport{})) queue_[count_++] = report;
    headFailed_ = false;
    // Keep the existing time gate: overflow must not cause a retry burst.
    return;
  }
  queue_[(head_ + count_) % keyboardConfig::reportQueueCapacity] = report;
  ++count_;
}

void KeyboardSession::update(const KeyboardLinkSnapshot& link, const KeyboardState& physical,
                             const UsageMatrix& layout, uint32_t nowMs) {
  const bool boundary = !initialized_ || link.generation != link_.generation;
  if (boundary) {
    head_ = count_ = 0;
    wanted_ = accepted_ = KeyboardReport{};
    localValid_ = false;
    initialEmpty_ = link.ready();
    inFlight_ = false;
    headFailed_ = false;
    waitMs_ = keyboardConfig::sendIntervalMs;
    initialized_ = true;
    // lastAttempt_ persists across sessions to cap the total call frequency.
  }
  link_ = link;
  blocked_ = false;
  PhysicalMatrix eligible = {};
  for (unsigned row = 0; row < 8; ++row) {
    for (unsigned col = 0; col < 18; ++col) {
      if ((boundary || !link.ready()) && (physical.raw()[row][col] || physical.stable()[row][col]))
        suppressed_[row][col] = true;
      if (suppressed_[row][col] && physical.releasedForDebounce(row, col, nowMs))
        suppressed_[row][col] = false;
      eligible[row][col] = link.ready() && physical.stable()[row][col] && !suppressed_[row][col];
    }
  }
  const KeyboardReport next = buildKeyboardReport(eligible, layout, wanted_);
  if (!sameKeyboardReport(next, wanted_)) {
    wanted_ = next;
    increment(totals_.wantedChanges);
    if (link.ready()) enqueue(wanted_);
  }
  // Preserve final correction even after history compaction or local rejection.
  if (link.ready() && !initialEmpty_ && !count_ &&
      (!localValid_ || !sameKeyboardReport(wanted_, accepted_))) enqueue(wanted_);
}

bool KeyboardSession::nextAttempt(uint32_t nowMs, Request& request) {
  if (!link_.ready() || blocked_ || inFlight_ || (!initialEmpty_ && !count_)) return false;
  if (attempted_ && uint32_t(nowMs - lastAttempt_) < waitMs_) return false;
  request.generation = link_.generation;
  request.token = ++token_;
  request.initialEmpty = initialEmpty_;
  request.report = initialEmpty_ ? KeyboardReport{} : queue_[head_];
  inFlight_ = attempted_ = true;
  lastAttempt_ = nowMs;
  increment(totals_.attempts);
  if (headFailed_) increment(totals_.retries);
  return true;
}

void KeyboardSession::complete(const Request& request, KeyboardSendResult result,
                               const KeyboardLinkSnapshot& afterSend) {
  if (!inFlight_ || request.token != token_ || request.generation != link_.generation) {
    increment(totals_.stale);
    return; // Late result cannot complete a newer request.
  }
  inFlight_ = false;
  if (afterSend.generation != link_.generation || !afterSend.ready() ||
      result == KeyboardSendResult::StaleSession || result == KeyboardSendResult::Disconnected ||
      result == KeyboardSendResult::NotReady) {
    increment(totals_.stale);
    blocked_ = true; // Wait for the next physical-frame/link update, never spin here.
    return;
  }
  if (result == KeyboardSendResult::LocalAccepted) {
    accepted_ = request.report;
    localValid_ = true;
    if (request.initialEmpty) initialEmpty_ = false;
    else { head_ = (head_ + 1) % keyboardConfig::reportQueueCapacity; --count_; }
    headFailed_ = false;
    waitMs_ = keyboardConfig::sendIntervalMs;
  } else {
    increment(totals_.rejected);
    waitMs_ = headFailed_ ? (waitMs_ > keyboardConfig::retryMaxMs / 2 ? keyboardConfig::retryMaxMs : waitMs_ * 2)
                         : keyboardConfig::retryInitialMs;
    headFailed_ = true; // No attempt limit: keep release/empty correction pending.
  }
}

KeyboardSession::Metrics KeyboardSession::metrics() const {
  Metrics result = totals_;
  result.link = link_;
  result.pending = count_;
  result.initialEmpty = initialEmpty_;
  result.localValid = localValid_;
  for (unsigned row = 0; row < 8; ++row)
    for (unsigned col = 0; col < 18; ++col) result.suppressed += suppressed_[row][col];
  return result;
}
