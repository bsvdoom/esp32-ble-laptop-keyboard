#pragma once
#include "KeyboardReport.h"

// Fixed laptop matrix, not a generic keyboard framework.
using PhysicalMatrix = bool[8][18];
using UsageMatrix = uint8_t[8][18];

// Retain held previous usages in their slots, then fill holes in matrix order.
KeyboardReport buildKeyboardReport(const PhysicalMatrix& stable, const UsageMatrix& layout,
                                   const KeyboardReport& previous);

class KeyboardState {
public:
  struct ScanResult {
    uint16_t stableChanges = 0;
    bool reportChanged = false;
  };

  // Called once with a complete frame and its end timestamp. No BLE/send outcome.
  ScanResult processScan(const PhysicalMatrix& samples, uint32_t nowMs, const UsageMatrix& layout);
  // Loop-owned snapshots for the later reconnect policy; callbacks must not read them.
  const PhysicalMatrix& raw() const { return raw_; }
  const PhysicalMatrix& stable() const { return stable_; }
  const KeyboardReport& desiredReport() const { return desired_; }
  // Also handles suppression of a raw press which never became stable HIGH.
  bool releasedForDebounce(unsigned row, unsigned col, uint32_t nowMs) const;

private:
  PhysicalMatrix raw_ = {};
  PhysicalMatrix stable_ = {};
  uint32_t changedAt_[8][18] = {};
  KeyboardReport desired_;
};
