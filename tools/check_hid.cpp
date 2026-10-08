#include "../include/KeyboardLayout.h"
#include "../include/KeyboardState.h"
#include "step2_hid_expected.h"
#include <cassert>
#include <cstdio>

int main() {
  const uint8_t modifierBits[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
  // All 144 physical cells retain the frozen, actually decoded 2/8 map.
  for (unsigned row = 0; row < 8; ++row) {
    for (unsigned col = 0; col < 18; ++col) {
      const uint8_t usage = expected[row][col];
      assert(keyboardLayout::matrix[row][col] == usage);
      PhysicalMatrix stable = {};
      stable[row][col] = true;
      const KeyboardReport empty;
      const auto report = buildKeyboardReport(stable, keyboardLayout::matrix, empty);
      assert(report.reserved == 0);
      if (usage >= 0xe0 && usage <= 0xe7) {
        assert(report.modifiers == modifierBits[usage - 0xe0]);
        assert(report.keys[0] == 0);
      } else {
        assert(report.modifiers == 0); // No implicit Shift/AltGr conversion.
        assert(report.keys[0] == usage);
      }
      for (unsigned slot = 1; slot < 6; ++slot) assert(report.keys[slot] == 0);
      stable[row][col] = false;
      const auto released = buildKeyboardReport(stable, keyboardLayout::matrix, report);
      assert(released.modifiers == 0 && released.reserved == 0);
      for (auto key : released.keys) assert(key == 0);
    }
  }
  std::puts("PASS: all 144 HU usage mappings and modifier bits through the full report builder");
}
