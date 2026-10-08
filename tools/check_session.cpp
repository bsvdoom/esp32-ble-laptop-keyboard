#include "KeyboardSession.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

static_assert(keyboardConfig::debounceMs == 20 && keyboardConfig::reportQueueCapacity == 8 &&
              keyboardConfig::sendIntervalMs == 20 && keyboardConfig::retryInitialMs == 20 &&
              keyboardConfig::retryMaxMs == 200, "Update literal traces when policy parameters change");
const UsageMatrix layout = {{0x04, 0x05, 0x06, 0xe0, 0x04, 0xe6}};

void expect(const KeyboardReport& report, uint8_t modifiers, std::initializer_list<uint8_t> keys) {
  assert(report.modifiers == modifiers && report.reserved == 0);
  unsigned slot = 0;
  for (uint8_t key : keys) { assert(slot < 6); assert(report.keys[slot++] == key); }
  while (slot < 6) assert(report.keys[slot++] == 0);
}

struct Rig {
  KeyboardState physical;
  KeyboardLinkState link;
  KeyboardSession session;
  PhysicalMatrix raw = {};
  void ready() {
    link.connect(7, false, false, false);
    link.security(7, true, false, true); // Encryption gate unchanged; no invented MITM.
    link.subscription(7, true, true, false, true);
  }
  void scan(uint32_t now) {
    physical.processScan(raw, now, layout);
    session.update(link.snapshot(), physical, layout, now);
  }
  KeyboardSession::Request take(uint32_t now, uint8_t modifiers, std::initializer_list<uint8_t> keys) {
    KeyboardSession::Request request;
    assert(session.nextAttempt(now, request));
    assert(request.generation == link.snapshot().generation);
    expect(request.report, modifiers, keys);
    return request;
  }
  void noAttempt(uint32_t now) {
    KeyboardSession::Request request;
    assert(!session.nextAttempt(now, request));
  }
  // Controlled local send substitute: same firmware completion path, no fake host ACK.
  void finish(const KeyboardSession::Request& request, KeyboardSendResult result) {
    session.complete(request, result, link.snapshot());
  }
  void send(uint32_t now, uint8_t modifiers, std::initializer_list<uint8_t> keys,
            KeyboardSendResult result = KeyboardSendResult::LocalAccepted) {
    finish(take(now, modifiers, keys), result);
  }
};

void heldKeysAndStableRelease() {
  Rig r;
  r.raw[0][0] = r.raw[0][3] = true;
  r.scan(0);
  r.link.connect(7, false, false, false);
  r.scan(10);
  r.scan(20);
  assert(r.physical.stable()[0][0] && r.physical.stable()[0][3]);
  r.link.security(7, true, false, true);
  r.link.subscription(7, true, true, false, true);
  r.scan(30);
  assert(r.session.isSuppressed(0, 0) && r.session.isSuppressed(0, 3));
  r.send(30, 0, {});
  r.scan(50); r.noAttempt(50);
  r.raw[0][0] = r.raw[0][3] = false; r.scan(55);
  r.raw[0][0] = r.raw[0][3] = true; r.scan(60);
  r.raw[0][0] = r.raw[0][3] = false; r.scan(70);
  r.scan(89);
  assert(r.session.isSuppressed(0, 0) && r.session.isSuppressed(0, 3));
  r.scan(90);
  assert(!r.session.isSuppressed(0, 0) && !r.session.isSuppressed(0, 3));
  r.raw[0][0] = r.raw[0][3] = true; r.scan(100); r.scan(120);
  r.send(120, 0x01, {0x04});
  r.link.disconnect(7); r.scan(130); r.noAttempt(130);
  r.ready(); r.scan(140); r.send(140, 0, {});
  r.scan(145); expect(r.session.wantedReport(), 0, {});
  assert(r.physical.stable()[0][0] && r.session.isSuppressed(0, 0));
  r.raw[0][0] = r.raw[0][3] = false; r.scan(150); r.scan(170);
  r.raw[0][0] = r.raw[0][3] = true; r.scan(180); r.scan(200);
  r.send(200, 0x01, {0x04});
}

void pendingRawAndDuplicateCells() {
  Rig r; r.ready(); r.scan(0); r.send(0, 0, {});
  r.raw[0][0] = true;
  r.link.subscription(7, false, true, false, true);
  r.link.subscription(7, true, true, false, true);
  r.scan(10); r.scan(20); r.send(20, 0, {});
  assert(r.physical.raw()[0][0] && !r.physical.stable()[0][0]);
  r.raw[0][0] = false; r.scan(25);
  r.raw[0][0] = true; r.scan(30);
  r.raw[0][0] = false; r.scan(35); r.scan(54);
  assert(r.session.isSuppressed(0, 0)); // Stable was already LOW, bounce must not unblock.
  r.scan(55); assert(!r.session.isSuppressed(0, 0));
  r.raw[0][0] = true; r.scan(60); r.scan(80); r.send(80, 0, {0x04});
  r.link.subscription(7, false, true, false, true);
  r.link.subscription(7, true, true, false, true);
  r.scan(90); r.scan(100); r.send(100, 0, {});
  // Same usage from a different, newly pressed physical cell remains usable.
  r.raw[0][4] = true; r.scan(110); r.scan(130); r.send(130, 0, {0x04});
  assert(r.session.isSuppressed(0, 0) && !r.session.isSuppressed(0, 4));
  r.raw[0][4] = false; r.scan(140); r.scan(160); r.send(160, 0, {});
  assert(r.physical.stable()[0][0]);
}

void offlineAndLateReadiness() {
  Rig r;
  r.raw[0][0] = true; r.scan(0); r.scan(20);
  r.raw[0][0] = false; r.scan(30); r.scan(50);
  r.noAttempt(50); r.ready(); r.scan(60); r.send(60, 0, {});
  r.scan(80); r.noAttempt(80);

  Rig late;
  late.link.connect(7, false, false, false); late.scan(0);
  late.raw[0][0] = true; late.scan(10); late.scan(30);
  late.raw[0][0] = false; late.scan(40); late.scan(60);
  late.link.subscription(7, true, false, false, true); late.scan(70); late.noAttempt(70);
  assert(late.link.snapshot().connected && !late.link.snapshot().ready());
  late.link.security(7, true, false, true); late.scan(80); late.send(80, 0, {});
  late.scan(100); late.noAttempt(100); expect(late.session.wantedReport(), 0, {});
}

void generationAndReadinessEdges() {
  Rig r; r.ready(); r.scan(0); r.send(0, 0, {});
  r.raw[0][0] = true; r.scan(30); r.scan(50); r.send(50, 0, {0x04});
  const auto old = r.link.snapshot();
  r.link.disconnect(7); r.ready(); // No intervening loop, same handle.
  assert(r.link.snapshot().handle == old.handle && r.link.snapshot().generation != old.generation);
  r.scan(60); r.noAttempt(60); r.scan(70); r.send(70, 0, {});
  assert(r.session.isSuppressed(0, 0));
  auto generation = r.link.snapshot().generation;
  r.link.subscription(7, false, true, false, true);
  r.link.subscription(7, true, true, false, true);
  assert(r.link.snapshot().generation != generation);
  r.scan(80); r.noAttempt(80); r.scan(90); r.send(90, 0, {});
  r.link.security(7, false, false, true); r.scan(100); r.noAttempt(100);
  assert(r.link.snapshot().connected && r.link.snapshot().subscribed && !r.link.snapshot().ready());
  r.link.security(7, true, false, true); r.scan(110); r.send(110, 0, {});
  generation = r.link.snapshot().generation;
  r.link.disconnect(99); r.link.security(99, false, false, false);
  r.link.subscription(99, false, false, false, false);
  assert(r.link.snapshot().generation == generation && r.link.snapshot().ready());
}

void initialEmptyRetryAndShortPress() {
  Rig r; r.ready(); r.scan(0);
  r.send(0, 0, {}, KeyboardSendResult::LocalRejected);
  assert(r.session.metrics().initialEmpty && !r.session.metrics().localValid);
  r.raw[0][0] = true; r.scan(5); r.noAttempt(19);
  r.scan(20); r.send(20, 0, {}, KeyboardSendResult::LocalRejected);
  r.scan(25); assert(r.session.metrics().pending == 1);
  r.raw[0][0] = false; r.scan(30); r.scan(50);
  assert(r.session.metrics().pending == 2); r.noAttempt(59);
  r.scan(60); r.send(60, 0, {});
  assert(!r.session.metrics().initialEmpty);
  r.scan(79); r.noAttempt(79); r.scan(80); r.send(80, 0, {0x04});
  r.scan(100); r.send(100, 0, {});
  r.scan(120); r.noAttempt(120);
  expect(r.session.locallyAcceptedReport(), 0, {});
}

void rejectedPressAndPersistentRelease() {
  Rig r; r.ready(); r.scan(0); r.send(0, 0, {});
  r.raw[0][0] = true; r.scan(30); r.scan(50);
  r.send(50, 0, {0x04}, KeyboardSendResult::LocalRejected);
  r.noAttempt(69); r.scan(70); r.send(70, 0, {0x04}, KeyboardSendResult::LocalRejected);
  r.raw[0][0] = false; r.scan(75); r.scan(95);
  assert(r.session.metrics().pending == 2); r.noAttempt(109);
  r.scan(110); r.send(110, 0, {0x04});
  for (uint32_t t : {130u, 150u, 190u, 270u, 430u, 630u}) {
    r.scan(t); r.send(t, 0, {}, KeyboardSendResult::LocalRejected);
    assert(r.session.metrics().pending == 1);
  }
  r.noAttempt(829); r.scan(830); r.send(830, 0, {});
  r.scan(850); r.noAttempt(850); expect(r.session.locallyAcceptedReport(), 0, {});
  assert(r.session.metrics().rejected == 8 && r.session.metrics().retries == 8);
}

void overflowRestoresFinalState() {
  Rig r; r.ready(); r.scan(0); r.send(0, 0, {});
  r.raw[0][0] = true; r.scan(1); r.scan(21);
  r.send(21, 0, {0x04}, KeyboardSendResult::LocalRejected);
  // No servicing during this synthetic overload; eight different full reports fill the fixed queue.
  for (uint32_t t = 40; t <= 320; t += 40) {
    r.raw[0][0] = !r.raw[0][0]; r.scan(t); r.scan(t + 20);
  }
  assert(r.session.metrics().overflows == 1 && r.session.metrics().discarded == 8);
  assert(r.session.metrics().pending == 2);
  expect(r.session.wantedReport(), 0, {0x04});
  r.send(400, 0, {}, KeyboardSendResult::LocalRejected); // Overflow release barrier stays pending.
  r.noAttempt(419); r.send(420, 0, {}); r.send(440, 0, {0x04});
  r.raw[0][0] = false; r.scan(450); r.scan(470); r.send(470, 0, {});
  r.scan(490); r.noAttempt(490); expect(r.session.locallyAcceptedReport(), 0, {});
}

void staleResultsAndNotReady() {
  Rig r; r.ready(); r.scan(0);
  const auto old = r.take(0, 0, {});
  r.link.disconnect(7); r.ready();
  r.finish(old, KeyboardSendResult::LocalAccepted); // Accepted API result after a different session began.
  assert(r.session.metrics().stale == 1 && !r.session.metrics().localValid);
  r.scan(1); r.noAttempt(1); r.scan(20);
  const auto current = r.take(20, 0, {});
  r.finish(old, KeyboardSendResult::LocalAccepted);
  r.noAttempt(40); // Late old result did not complete the new in-flight request.
  r.finish(current, KeyboardSendResult::LocalAccepted);
  assert(r.session.metrics().localValid && !r.session.metrics().initialEmpty);
  r.raw[0][0] = true; r.scan(50); r.scan(70);
  r.send(70, 0, {0x04}, KeyboardSendResult::NotReady);
  r.noAttempt(100); // Requires another frame/link update, no completion-driven spin.
  r.link.subscription(7, false, true, false, true); r.scan(110); r.noAttempt(200);
  r.link.subscription(7, true, true, false, true); r.scan(210); r.send(210, 0, {});
  assert(r.session.isSuppressed(0, 0) && r.session.metrics().pending == 0);
  r.raw[0][0] = false; r.scan(220); r.scan(240);
  r.raw[0][0] = true; r.scan(250); r.scan(270);
  const auto pending = r.take(270, 0, {0x04});
  r.link.disconnect(7); r.finish(pending, KeyboardSendResult::Disconnected);
  r.scan(280); r.noAttempt(400);
}

void wrappingRetryClock() {
  Rig r; r.ready(); r.scan(0xfffffff8u);
  r.send(0xfffffff8u, 0, {}, KeyboardSendResult::LocalRejected);
  r.scan(0x0bu); r.noAttempt(0x0bu);
  r.scan(0x0cu); r.send(0x0cu, 0, {});
  r.raw[0][0] = true; r.scan(20); r.scan(40); r.send(40, 0, {0x04});
  r.raw[0][0] = false; r.scan(50); r.scan(70);
  r.send(70, 0, {}, KeyboardSendResult::LocalRejected);
  r.noAttempt(89); r.scan(90); r.send(90, 0, {});
}

int main() {
  heldKeysAndStableRelease();
  pendingRawAndDuplicateCells();
  offlineAndLateReadiness();
  generationAndReadinessEdges();
  initialEmptyRetryAndShortPress();
  rejectedPressAndPersistentRelease();
  overflowRestoresFinalState();
  staleResultsAndNotReady();
  wrappingRetryClock();
  std::puts("PASS: held/modifiers, pending raw, duplicates, offline, generations/readiness, empty sync, timed retries, short press, overflow, stale results, wrap");
}
