#pragma once
#include <stdint.h>

// Report ID 1 is in the GATT report reference, not prefixed to these 8 bytes.
struct KeyboardReport {
  uint8_t modifiers = 0;
  uint8_t reserved = 0;
  uint8_t keys[6] = {};
};
static_assert(sizeof(KeyboardReport) == 8, "HID keyboard report must be 8 bytes");

inline bool sameKeyboardReport(const KeyboardReport& a, const KeyboardReport& b) {
  if (a.modifiers != b.modifiers || a.reserved != b.reserved) return false;
  for (unsigned slot = 0; slot < 6; ++slot)
    if (a.keys[slot] != b.keys[slot]) return false;
  return true;
}
