#include "../include/KeyboardConfig.h"
#include "../include/KeyboardState.h"
#include <cassert>
#include <initializer_list>
#include <cstdio>

static_assert(keyboardConfig::debounceMs == 20, "Update literal trace timestamps if the threshold changes");
// A..G; all eight modifiers; null; duplicate A; duplicate left Ctrl.
const UsageMatrix layout = {
  {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
   0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0, 0x04, 0xe0}
};

void expectReport(const KeyboardReport& report, uint8_t modifiers,
                  std::initializer_list<uint8_t> keys) {
  assert(report.modifiers == modifiers);
  assert(report.reserved == 0);
  assert(keys.size() <= 6);
  unsigned slot = 0;
  for (const auto key : keys) assert(report.keys[slot++] == key);
  while (slot < 6) assert(report.keys[slot++] == 0);
}

void expectScan(const KeyboardState::ScanResult& scan, unsigned changes, bool reportChanged) {
  assert(scan.stableChanges == changes);
  assert(scan.reportChanged == reportChanged);
}

void bouncingAndIndependentKeys() {
  KeyboardState state;
  PhysicalMatrix raw = {};
  expectScan(state.processScan(raw, 0, layout), 0, false);
  raw[0][0] = true;
  expectScan(state.processScan(raw, 100, layout), 0, false);
  assert(state.raw()[0][0] && !state.stable()[0][0]);
  raw[0][0] = false;
  expectScan(state.processScan(raw, 105, layout), 0, false);
  raw[0][0] = raw[0][1] = true;
  expectScan(state.processScan(raw, 110, layout), 0, false);
  raw[0][0] = false;
  expectScan(state.processScan(raw, 115, layout), 0, false);
  raw[0][0] = true;
  expectScan(state.processScan(raw, 120, layout), 0, false);
  expectScan(state.processScan(raw, 129, layout), 0, false);
  // B settles while A is still being debounced; no matrix-wide timer.
  expectScan(state.processScan(raw, 130, layout), 1, true);
  assert(!state.stable()[0][0] && state.stable()[0][1]);
  expectReport(state.desiredReport(), 0, {0x05});
  expectScan(state.processScan(raw, 139, layout), 0, false);
  expectScan(state.processScan(raw, 140, layout), 1, true);
  expectReport(state.desiredReport(), 0, {0x05, 0x04});
  // B's release bounces too, while A remains stable.
  raw[0][1] = false;
  expectScan(state.processScan(raw, 150, layout), 0, false);
  raw[0][1] = true;
  expectScan(state.processScan(raw, 155, layout), 0, false);
  raw[0][1] = false;
  expectScan(state.processScan(raw, 160, layout), 0, false);
  expectScan(state.processScan(raw, 179, layout), 0, false);
  expectReport(state.desiredReport(), 0, {0x05, 0x04});
  expectScan(state.processScan(raw, 180, layout), 1, true);
  expectReport(state.desiredReport(), 0, {0, 0x04}); // A keeps its original slot.
  raw[0][0] = false;
  expectScan(state.processScan(raw, 1000, layout), 0, false);
  expectScan(state.processScan(raw, 1019, layout), 0, false);
  expectScan(state.processScan(raw, 1020, layout), 1, true);
  expectReport(state.desiredReport(), 0, {});
}

void wrappingTimestamps() {
  KeyboardState press;
  PhysicalMatrix raw = {};
  raw[0][0] = true;
  expectScan(press.processScan(raw, 0xfffffff8u, layout), 0, false);
  expectScan(press.processScan(raw, 0x0000000bu, layout), 0, false); // 19 ms
  expectScan(press.processScan(raw, 0x0000000cu, layout), 1, true);  // 20 ms
  expectReport(press.desiredReport(), 0, {0x04});

  KeyboardState release;
  expectScan(release.processScan(raw, 0, layout), 0, false); // Zero is a valid start time.
  expectScan(release.processScan(raw, 20, layout), 1, true);
  raw[0][0] = false;
  expectScan(release.processScan(raw, 0xfffffff8u, layout), 0, false);
  expectScan(release.processScan(raw, 0x0000000bu, layout), 0, false);
  assert(!release.raw()[0][0] && release.stable()[0][0]);
  expectScan(release.processScan(raw, 0x0000000cu, layout), 1, true);
  expectReport(release.desiredReport(), 0, {});
}

void sixKeysAndModifiers() {
  KeyboardState state;
  PhysicalMatrix raw = {};
  for (unsigned col = 0; col < 6; ++col) raw[0][col] = true;
  expectScan(state.processScan(raw, 0, layout), 0, false);
  expectScan(state.processScan(raw, 20, layout), 6, true);
  expectReport(state.desiredReport(), 0, {0x04, 0x05, 0x06, 0x07, 0x08, 0x09});
  for (unsigned col = 7; col < 15; ++col) raw[0][col] = true;
  expectScan(state.processScan(raw, 30, layout), 0, false);
  expectScan(state.processScan(raw, 50, layout), 8, true);
  expectReport(state.desiredReport(), 0xff, {0x04, 0x05, 0x06, 0x07, 0x08, 0x09});
  for (unsigned col = 7; col < 15; ++col) raw[0][col] = false;
  expectScan(state.processScan(raw, 60, layout), 0, false);
  expectScan(state.processScan(raw, 80, layout), 8, true);
  expectReport(state.desiredReport(), 0, {0x04, 0x05, 0x06, 0x07, 0x08, 0x09});
}

void seventhHeldKeyAndNoEviction() {
  KeyboardState state;
  PhysicalMatrix raw = {};
  // Seven simultaneous usages choose the first six in row/column order.
  for (unsigned col = 0; col < 7; ++col) raw[0][col] = true;
  expectScan(state.processScan(raw, 0, layout), 0, false);
  expectScan(state.processScan(raw, 20, layout), 7, true);
  expectReport(state.desiredReport(), 0, {0x04, 0x05, 0x06, 0x07, 0x08, 0x09});
  assert(state.stable()[0][6]); // G remains physically down even though omitted.
  raw[0][0] = false;
  expectScan(state.processScan(raw, 30, layout), 0, false);
  expectScan(state.processScan(raw, 50, layout), 1, true);
  expectReport(state.desiredReport(), 0, {0x0a, 0x05, 0x06, 0x07, 0x08, 0x09});
  // A now has earlier matrix priority, but cannot evict retained G.
  raw[0][0] = true;
  expectScan(state.processScan(raw, 60, layout), 0, false);
  expectScan(state.processScan(raw, 80, layout), 1, false);
  expectReport(state.desiredReport(), 0, {0x0a, 0x05, 0x06, 0x07, 0x08, 0x09});
  raw[0][1] = false;
  expectScan(state.processScan(raw, 90, layout), 0, false);
  expectScan(state.processScan(raw, 110, layout), 1, true);
  expectReport(state.desiredReport(), 0, {0x0a, 0x04, 0x06, 0x07, 0x08, 0x09});
}

void nullAndDuplicateCells() {
  KeyboardState state;
  PhysicalMatrix raw = {};
  raw[0][0] = raw[0][16] = raw[0][15] = raw[0][7] = raw[0][17] = true;
  expectScan(state.processScan(raw, 100, layout), 0, false);
  expectScan(state.processScan(raw, 120, layout), 5, true);
  expectReport(state.desiredReport(), 0x01, {0x04});
  assert(state.raw()[0][15] && state.stable()[0][15]); // Null remains physical only.
  raw[0][0] = raw[0][7] = false;
  expectScan(state.processScan(raw, 140, layout), 0, false);
  expectScan(state.processScan(raw, 160, layout), 2, false);
  expectReport(state.desiredReport(), 0x01, {0x04}); // Other A/Ctrl cells still held.
  raw[0][16] = raw[0][17] = false;
  expectScan(state.processScan(raw, 180, layout), 0, false);
  expectScan(state.processScan(raw, 200, layout), 2, true);
  expectReport(state.desiredReport(), 0, {});
  raw[0][15] = false;
  expectScan(state.processScan(raw, 220, layout), 0, false);
  expectScan(state.processScan(raw, 240, layout), 1, false);
  expectReport(state.desiredReport(), 0, {});
}

void completeFramesWithoutBle() {
  KeyboardState state; // Exactly the firmware processor; has no BLE/connected input.
  PhysicalMatrix raw = {};
  raw[0][0] = raw[0][1] = true;
  expectScan(state.processScan(raw, 0, layout), 0, false);
  expectScan(state.processScan(raw, 20, layout), 2, true);
  expectReport(state.desiredReport(), 0, {0x04, 0x05});
  // A send cannot update this state: Disconnected/NotReady/rejected have no input here.
  expectScan(state.processScan(raw, 25, layout), 0, false);
  assert(state.raw()[0][0] && state.stable()[0][0]);
  expectReport(state.desiredReport(), 0, {0x04, 0x05});
  // Two releases + one press in one complete frame yield only the final C report.
  raw[0][0] = raw[0][1] = false;
  raw[0][2] = true;
  expectScan(state.processScan(raw, 30, layout), 0, false);
  expectReport(state.desiredReport(), 0, {0x04, 0x05});
  expectScan(state.processScan(raw, 50, layout), 3, true);
  expectReport(state.desiredReport(), 0, {0x06});
  expectScan(state.processScan(raw, 60, layout), 0, false);
  expectReport(state.desiredReport(), 0, {0x06});
}

int main() {
  bouncingAndIndependentKeys();
  wrappingTimestamps();
  sixKeysAndModifiers();
  seventhHeldKeyAndNoEviction();
  nullAndDuplicateCells();
  completeFramesWithoutBle();
  std::puts("PASS: bounce/independence, wrap, 6+modifiers, rollover selection, null/duplicates, offline/full frames");
}
