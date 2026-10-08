#include "BatteryState.h"
#include "KeyboardLink.h"
#include <cassert>
#include <cstdio>

static BatterySample sample(int percent, uint32_t now, bool valid = true) {
  BatterySample result;
  result.percent = percent; result.measuredMs = now; result.valid = valid;
  return result;
}

int main() {
  uint8_t value = 77;
  assert(batteryLevel(sample(0, 10), 10, value) && value == 0);
  assert(batteryLevel(sample(42, 10), 10, value) && value == 42);
  assert(batteryLevel(sample(100, 10), 10, value) && value == 100);
  assert(!batteryLevel(sample(-1, 10), 10, value));
  assert(!batteryLevel(sample(101, 10), 10, value));
  assert(!batteryLevel(BatterySample{}, 10, value));
  assert(!batteryLevel(sample(42, 10, false), 10, value));
  assert(batteryLevel(sample(42, 10), 60010, value));
  assert(!batteryLevel(sample(42, 10), 60011, value));
  assert(!batteryLevel(sample(42, 11), 10, value)); // Future clock is not a fresh reading.
  assert(batteryLevel(sample(42, 0xfffffff0u), 0x20, value));
  assert(!batteryLevel(sample(42, 0xfffffff0u), 0xea51, value)); // 60001 ms old, wrapped.

  BatteryLink link;
  BatteryState state;
  BatteryState::Request request;
  state.update(sample(42, 0), link, 0);
  assert(!state.nextAttempt(0, request)); // Offline data never queues historical events.
  link.connect(7, false);
  state.update(sample(42, 1), link, 1); assert(!state.nextAttempt(1, request));
  link.subscription(7, true);
  state.update(sample(42, 2), link, 2); assert(!state.nextAttempt(2, request));
  link.security(7, true);
  state.update(sample(42, 3), link, 3);
  assert(state.nextAttempt(3, request) && request.percent == 42 && request.handle == 7);
  state.complete(request, false, link); // Controlled local rejection, not a host ACK.
  assert(!state.nextAttempt(1002, request));
  assert(state.nextAttempt(1003, request) && request.percent == 42);
  state.complete(request, true, link);
  state.update(sample(42, 2003), link, 2003); assert(!state.nextAttempt(2003, request));
  state.update(sample(100, 2003), link, 2003);
  assert(state.nextAttempt(2003, request) && request.percent == 100);
  state.complete(request, true, link);
  state.update(sample(0, 3003), link, 3003);
  assert(state.nextAttempt(3003, request) && request.percent == 0);
  state.complete(request, false, link);
  state.update(sample(101, 4003), link, 4003); assert(!state.nextAttempt(4003, request));
  state.update(BatterySample{}, link, 4003); assert(!state.nextAttempt(4003, request));
  state.update(sample(55, 0), link, 60001); assert(!state.nextAttempt(60001, request));
  state.update(sample(55, 60002), link, 60002);
  assert(state.nextAttempt(60002, request) && request.percent == 55); // Latest replaces rejected 0.
  const auto old = request;
  link.disconnect(7); link.connect(7, true); link.subscription(7, true);
  state.update(sample(23, 60003), link, 60003);
  state.complete(old, true, link);
  assert(!state.nextAttempt(61001, request)); // Rate limit spans reconnect, stale result ignored.
  assert(state.nextAttempt(61002, request) && request.percent == 23 && request.generation != old.generation);
  state.complete(request, true, link);
  link.subscription(7, false);
  state.update(sample(24, 62002), link, 62002); assert(!state.nextAttempt(62002, request));
  link.subscription(7, true);
  state.update(sample(24, 62003), link, 62003);
  assert(state.nextAttempt(62003, request) && request.percent == 24);
  state.complete(request, true, link);
  // Battery subscription cannot touch keyboard readiness/generation.
  KeyboardLinkState keyboard;
  keyboard.connect(7, true, false, true);
  const auto keyboardBefore = keyboard.snapshot();
  link.subscription(7, false);
  assert(keyboard.snapshot().generation == keyboardBefore.generation && !keyboard.snapshot().ready());
  keyboard.subscription(7, true, true, false, true);
  const auto batteryGeneration = link.generation;
  keyboard.subscription(7, false, true, false, true);
  assert(link.generation == batteryGeneration);

  BatteryState wrap;
  BatteryLink wrapLink; wrapLink.connect(1, true); wrapLink.subscription(1, true);
  wrap.update(sample(88, 0xffffff00u), wrapLink, 0xffffff00u);
  assert(wrap.nextAttempt(0xffffff00u, request)); wrap.complete(request, false, wrapLink);
  assert(!wrap.nextAttempt(0x2e7u, request)); // 999 ms since rejection.
  assert(wrap.nextAttempt(0x2e8u, request) && request.percent == 88);
  wrap.complete(request, true, wrapLink);
  wrap.update(sample(88, 0x2e8u), wrapLink, 0x2e8u);
  assert(!wrap.nextAttempt(0x10000u, request)); // Stale samples never notify.
  std::puts("PASS battery: levels/invalid/stale/wrap, readiness/CCCD, latest retry/reconnect/isolation");
}
