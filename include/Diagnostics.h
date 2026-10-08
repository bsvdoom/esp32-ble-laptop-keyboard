#pragma once
#include <stdint.h>
#include "KeyboardSession.h"

namespace diagnostics {
void begin();
void poll();
void loopDuration(uint64_t microseconds);
void scanDuration(uint64_t microseconds);
// Loop-owned physical/report counters; no key values are logged.
void scanProcessed(uint16_t stableChanges, bool reportChanged);
void session(const KeyboardSession::Metrics&);
// Scalar observations only. Callback-safe, bounded queue, no Serial writes here.
void stackInit(bool ok);
void advertisingStart(bool ok);
void advertisingComplete(int reason);
void connect(uint16_t handle);
void disconnect(uint16_t handle, int reason);
void encryption(uint16_t handle, int code);
void authentication(uint16_t handle, bool encrypted, bool authenticated, bool bonded);
void subscription(uint16_t handle, uint16_t value);
void reportAttempt();
void reportSkipped(bool connected);
void notifyResult(bool accepted);
void notifyStatus(int code);
void ledOutput(uint16_t handle, bool valid, uint8_t value);
}
