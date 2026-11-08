#include "Diagnostics.h"
#include "KeyboardBuildInfo.h"
#include <Arduino.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <stdarg.h>

namespace {
enum class Kind : uint8_t { Connect, Disconnect, Auth, Subscribe, StatusError, Led, Encryption, StackInit, AdvertisingStart, AdvertisingComplete };
struct Event {
  uint64_t ms;
  Kind kind;
  uint16_t handle;
  uint8_t report;
  int value;
  int code;
};
struct Counters {
  uint32_t calls = 0, skipped = 0, notReady = 0, apiAccepted = 0, apiRejected = 0;
  uint32_t statusOk = 0, statusError = 0;
  uint32_t eventDrop = 0;
  int lastCode = -1;
  bool linkObserved = false;
};
constexpr uint8_t capacity = 16;
Event events[capacity];
uint8_t readPos = 0, count = 0;
Counters counters;
portMUX_TYPE diagnosticMux = portMUX_INITIALIZER_UNLOCKED;
// Loop-owned metrics, never accessed by callbacks.
uint64_t lastLoop = 0, maxLoop = 0, lastScan = 0, maxScan = 0;
uint32_t scans = 0, logDrop = 0, stableEdges = 0, reportChanges = 0;
uint64_t lastHeartbeat = 0;
KeyboardSession::Metrics sessionMetrics; // Loop-owned; no callback reads/writes.
keyboardble::OtaStatus otaSnapshot; // Loop-owned copy taken under the OTA mutex.

void increment(uint32_t& value) { if (value != UINT32_MAX) ++value; }

void enqueueLocked(const Event& event) {
  if (count == capacity) { increment(counters.eventDrop); return; }
  events[(readPos + count) % capacity] = event;
  ++count;
}

void record(Kind kind, uint16_t handle, uint8_t report, int value, int code = 0) {
  const Event event{static_cast<uint64_t>(esp_timer_get_time() / 1000), kind,
                    handle, report, value, code};
  portENTER_CRITICAL(&diagnosticMux);
  if (kind == Kind::Connect) counters.linkObserved = true;
  if (kind == Kind::Disconnect) counters.linkObserved = false;
  enqueueLocked(event);
  portEXIT_CRITICAL(&diagnosticMux);
}

// One bounded write, only when the UART FIFO/ring has enough room.
// No flush, retry, delay or wait for serial attachment.
void emit(const char* format, ...) __attribute__((format(printf, 1, 2)));
void emit(const char* format, ...) {
  char line[384];
  va_list args;
  va_start(args, format);
  const int length = vsnprintf(line, sizeof(line), format, args);
  va_end(args);
  if (length <= 0 || length >= static_cast<int>(sizeof(line)) ||
      Serial.availableForWrite() < length) {
    increment(logDrop);
    return;
  }
  Serial.write(reinterpret_cast<const uint8_t*>(line), length);
}

const char* resetName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "poweron";
    case ESP_RST_EXT: return "external";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt_wdt";
    case ESP_RST_TASK_WDT: return "task_wdt";
    case ESP_RST_WDT: return "other_wdt";
    case ESP_RST_DEEPSLEEP: return "deepsleep";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "sdio";
    default: return "unknown";
  }
}

const char* otaStateName(keyboardble::OtaState state) {
  switch (state) {
    case keyboardble::OtaState::Idle: return "idle";
    case keyboardble::OtaState::Receiving: return "receiving";
    case keyboardble::OtaState::RebootPending: return "reboot_pending";
  }
  return "unknown";
}

void emitOta(uint64_t now) {
  const auto& o = otaSnapshot;
  emit("OTA t_ms=%" PRIu64 " state=%s rx=%" PRIu32 "/%" PRIu32 " last_error=0x%02X sessions=%" PRIu32
       " failures=%" PRIu32 "\n",
       now, otaStateName(o.state), o.receivedBytes, o.totalBytes,
       static_cast<unsigned>(o.lastError), o.sessions, o.failures);
}
}

namespace diagnostics {
void begin() {
  const auto reason = esp_reset_reason();
  emit("BOOT id=%s built_utc=%s reset=%d/%s idf=%s\n",
       KEYBOARD_BUILD_ID, KEYBOARD_BUILD_UTC, static_cast<int>(reason),
       resetName(reason), esp_get_idf_version());
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* next = esp_ota_get_next_update_partition(nullptr);
  emit("OTA_PARTITION running=%s next=%s next_size=%" PRIu32 "\n",
       running ? running->label : "?", next ? next->label : "?",
       next ? static_cast<uint32_t>(next->size) : 0u);
}

void ota(const keyboardble::OtaStatus& status) {
  const bool changed = status.state != otaSnapshot.state || status.lastError != otaSnapshot.lastError ||
                       status.sessions != otaSnapshot.sessions || status.failures != otaSnapshot.failures;
  otaSnapshot = status;
  if (changed) emitOta(esp_timer_get_time() / 1000);
}

void loopDuration(uint64_t duration) {
  lastLoop = duration;
  if (duration > maxLoop) maxLoop = duration;
}

void scanDuration(uint64_t duration) {
  lastScan = duration;
  if (duration > maxScan) maxScan = duration;
  increment(scans);
}

void scanProcessed(uint16_t stableChanges, bool reportChanged) {
  // All accesses are in loop, separate from callback-shared counters.
  stableEdges += stableChanges > UINT32_MAX - stableEdges ? UINT32_MAX - stableEdges : stableChanges;
  if (reportChanged) increment(reportChanges);
}

void session(const KeyboardSession::Metrics& metrics) { sessionMetrics = metrics; }

void poll() {
  Event event{};
  bool present;
  portENTER_CRITICAL(&diagnosticMux);
  present = count != 0;
  if (present) { event = events[readPos]; readPos = (readPos + 1) % capacity; --count; }
  portEXIT_CRITICAL(&diagnosticMux);
  // At most one lifecycle/error event per loop. Lost output is counted, not retried.
  if (present) {
    const char* names[] = {"connect", "disconnect", "auth", "subscribe", "notify_status_error", "led",
                           "encryption", "stack_init", "advertising_start", "advertising_complete"};
    emit("BLE t_ms=%" PRIu64 " event=%s handle=%u report=%u value=%d code=%d\n",
         event.ms, names[static_cast<unsigned>(event.kind)], event.handle,
         event.report, event.value, event.code);
  }
  const uint64_t now = esp_timer_get_time() / 1000;
  if (now - lastHeartbeat < 10000) return;
  lastHeartbeat = now;
  Counters snapshot;
  portENTER_CRITICAL(&diagnosticMux);
  snapshot = counters;
  portEXIT_CRITICAL(&diagnosticMux);
  emit("HB t_ms=%" PRIu64 " link_obs=%u loop_us=%" PRIu64 " loop_max_us=%" PRIu64
       " scan_us=%" PRIu64 " scan_max_us=%" PRIu64 " scans=%" PRIu32 " stable_edges=%" PRIu32 " physical_report_changes=%" PRIu32
       " heap=%" PRIu32 " heap_min=%" PRIu32 " ev_drop=%" PRIu32 " log_drop=%" PRIu32 "\n",
       now, snapshot.linkObserved, lastLoop, maxLoop, lastScan, maxScan, scans, stableEdges, reportChanges,
       static_cast<uint32_t>(esp_get_free_heap_size()), static_cast<uint32_t>(esp_get_minimum_free_heap_size()),
       snapshot.eventDrop, logDrop);
  emit("TX t_ms=%" PRIu64 " notify_attempts=%" PRIu32 " skipped_disconnected=%" PRIu32 " skipped_not_ready=%" PRIu32
       " api_accepted=%" PRIu32 " api_rejected=%" PRIu32 " status_ok=%" PRIu32 " status_error=%" PRIu32 " last_status_code=%d\n",
       now, snapshot.calls, snapshot.skipped, snapshot.notReady, snapshot.apiAccepted,
       snapshot.apiRejected, snapshot.statusOk, snapshot.statusError, snapshot.lastCode);
  const auto& m = sessionMetrics;
  emit("SESSION gen=%" PRIu64 " gap=%u enc=%u auth=%u bond=%u sub=%u ready=%u"
       " suppressed=%u pending=%u sync=%u local_valid=%u wanted_changes=%" PRIu32
       " attempts=%" PRIu32 " retries=%" PRIu32 " rejected=%" PRIu32 " overflow=%" PRIu32 " discarded=%" PRIu32 " stale=%" PRIu32 "\n",
       m.link.generation, m.link.connected, m.link.encrypted, m.link.authenticated, m.link.bonded,
       m.link.subscribed, m.link.ready(), m.suppressed, m.pending, m.initialEmpty, m.localValid,
       m.wantedChanges, m.attempts, m.retries, m.rejected, m.overflows, m.discarded, m.stale);
  emitOta(now);
}
}

namespace diagnostics {
void stackInit(bool ok) { record(Kind::StackInit, UINT16_MAX, 0, ok); }
void advertisingStart(bool ok) { record(Kind::AdvertisingStart, UINT16_MAX, 0, ok); }
void advertisingComplete(int reason) { record(Kind::AdvertisingComplete, UINT16_MAX, 0, 0, reason); }
void connect(uint16_t handle) { record(Kind::Connect, handle, 0, 1); }
void disconnect(uint16_t handle, int reason) { record(Kind::Disconnect, handle, 0, 0, reason); }
void encryption(uint16_t handle, int code) { record(Kind::Encryption, handle, 0, 0, code); }
void authentication(uint16_t handle, bool encrypted, bool authenticated, bool bonded) {
  record(Kind::Auth, handle, 0, encrypted | (authenticated << 1) | (bonded << 2));
}
void subscription(uint16_t handle, uint16_t value) { record(Kind::Subscribe, handle, 1, value); }
void ledOutput(uint16_t handle, bool valid, uint8_t value) { record(Kind::Led, handle, 1, valid ? value : -1); }

void reportAttempt() {
  portENTER_CRITICAL(&diagnosticMux);
  increment(counters.calls);
  portEXIT_CRITICAL(&diagnosticMux);
}
void reportSkipped(bool connected) {
  portENTER_CRITICAL(&diagnosticMux);
  if (connected) increment(counters.notReady); else increment(counters.skipped);
  portEXIT_CRITICAL(&diagnosticMux);
}
void notifyResult(bool accepted) {
  portENTER_CRITICAL(&diagnosticMux);
  if (accepted) increment(counters.apiAccepted); else increment(counters.apiRejected);
  portEXIT_CRITICAL(&diagnosticMux);
}
void notifyStatus(int code) {
  const Event event{static_cast<uint64_t>(esp_timer_get_time() / 1000), Kind::StatusError,
                    UINT16_MAX, 1, 0, code};
  portENTER_CRITICAL(&diagnosticMux);
  counters.lastCode = code;
  if (code == 0) increment(counters.statusOk);
  else { increment(counters.statusError); enqueueLocked(event); }
  portEXIT_CRITICAL(&diagnosticMux);
}
}
