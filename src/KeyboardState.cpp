#include "KeyboardState.h"
#include "KeyboardConfig.h"

bool KeyboardState::releasedForDebounce(unsigned row, unsigned col, uint32_t nowMs) const {
  return !raw_[row][col] && !stable_[row][col] &&
         uint32_t(nowMs - changedAt_[row][col]) >= keyboardConfig::debounceMs;
}

KeyboardReport buildKeyboardReport(const PhysicalMatrix& stable, const UsageMatrix& layout,
                                   const KeyboardReport& previous) {
  KeyboardReport next;
  bool available[0x66] = {};
  for (unsigned row = 0; row < 8; ++row) {
    for (unsigned col = 0; col < 18; ++col) {
      if (!stable[row][col]) continue;
      const uint8_t usage = layout[row][col];
      if (usage >= 0xe0 && usage <= 0xe7)
        next.modifiers |= uint8_t(1u << (usage - 0xe0));
      else if (usage >= 0x04 && usage <= 0x65)
        available[usage] = true;
    }
  }
  // Previous *desired* state determines selection, never local notify success.
  for (unsigned slot = 0; slot < 6; ++slot) {
    const uint8_t usage = previous.keys[slot];
    if (usage >= 0x04 && usage <= 0x65 && available[usage]) {
      next.keys[slot] = usage;
      available[usage] = false; // Deduplicate even a malformed previous report.
    }
  }
  for (unsigned row = 0; row < 8; ++row) {
    for (unsigned col = 0; col < 18; ++col) {
      const uint8_t usage = layout[row][col];
      if (!stable[row][col] || usage < 0x04 || usage > 0x65 || !available[usage]) continue;
      for (auto& slot : next.keys) {
        if (slot == 0) { slot = usage; break; }
      }
      available[usage] = false;
    }
  }
  return next;
}

KeyboardState::ScanResult KeyboardState::processScan(const PhysicalMatrix& samples,
                                                    uint32_t nowMs, const UsageMatrix& layout) {
  ScanResult result;
  for (unsigned row = 0; row < 8; ++row) {
    for (unsigned col = 0; col < 18; ++col) {
      if (samples[row][col] != raw_[row][col]) {
        raw_[row][col] = samples[row][col];
        changedAt_[row][col] = nowMs;
      }
      // Unsigned elapsed time crosses a uint32_t wrap without a future deadline.
      if (raw_[row][col] != stable_[row][col] &&
          uint32_t(nowMs - changedAt_[row][col]) >= keyboardConfig::debounceMs) {
        stable_[row][col] = raw_[row][col];
        ++result.stableChanges;
      }
    }
  }
  const KeyboardReport next = buildKeyboardReport(stable_, layout, desired_);
  result.reportChanged = !sameKeyboardReport(next, desired_);
  desired_ = next;
  return result;
}
