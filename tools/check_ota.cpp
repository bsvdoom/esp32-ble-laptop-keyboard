// Native check for the portable Simple BLE OTA protocol state machine.
// Build: g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iinclude tools/check_ota.cpp src/OtaSession.cpp
#include "../include/OtaSession.h"
#include <cassert>
#include <cstdio>

using namespace keyboardble;

static void otaProtocolAndAuth() {
    struct FakeOta final : OtaBackend {
        uint32_t max = 1024, written = 0, aborts = 0;
        bool active = false;
        uint32_t maxFirmwareSize() const override { return max; }
        bool begin(uint32_t) override { active = true; written = 0; return true; }
        bool write(const uint8_t *, size_t len) override { written += uint32_t(len); return true; }
        bool end() override { active = false; return true; }
        void abort() override { if (active) { active = false; ++aborts; } }
    } backend;
    OtaSession ota;
    OtaError err = OtaError::None;
    const uint8_t start9[5] = {0x01, 9, 0, 0, 0};
    // Unauthenticated START rejected
    assert(ota.onControlWrite(start9, 5, false, backend, err) && err == OtaError::BeginFailed);
    // Oversize START rejected
    const uint8_t startBig[5] = {0x01, 0, 8, 0, 0}; // 2048 > 1024
    assert(ota.onControlWrite(startBig, 5, true, backend, err) && err == OtaError::PartitionFull);
    // Valid START + "123456789" (IEEE CRC32 = 0xCBF43926)
    assert(!ota.onControlWrite(start9, 5, true, backend, err) && ota.status().active());
    const uint8_t payload[] = {'1','2','3','4','5','6','7','8','9'};
    assert(!ota.onDataWrite(payload, 4, true, backend, err));
    assert(!ota.onDataWrite(payload + 4, 5, true, backend, err));
    assert(ota.status().crc32 == 0xCBF43926u && ota.status().receivedBytes == 9);
    const uint8_t completeOk[5] = {0x02, 0x26, 0x39, 0xF4, 0xCB};
    assert(!ota.onControlWrite(completeOk, 5, true, backend, err));
    assert(ota.status().state == OtaState::RebootPending && !backend.active);
    // Bad CRC aborts active session
    assert(!ota.onControlWrite(start9, 5, true, backend, err));
    assert(!ota.onDataWrite(payload, 9, true, backend, err));
    const uint8_t completeBad[5] = {0x02, 0x00, 0x39, 0xF4, 0xCB};
    assert(ota.onControlWrite(completeBad, 5, true, backend, err) && err == OtaError::CrcMismatch);
    assert(ota.status().state == OtaState::Idle && backend.aborts == 1);
}

static void disconnectAbortsAndRetries() {
    struct FakeOta final : OtaBackend {
        uint32_t aborts = 0;
        bool active = false;
        uint32_t maxFirmwareSize() const override { return 1024; }
        bool begin(uint32_t) override { active = true; return true; }
        bool write(const uint8_t *, size_t) override { return true; }
        bool end() override { active = false; return true; }
        void abort() override { if (active) { active = false; ++aborts; } }
    } backend;
    OtaSession ota;
    OtaError err = OtaError::None;
    const uint8_t start9[5] = {0x01, 9, 0, 0, 0};
    const uint8_t payload[] = {'1','2','3','4','5','6','7','8','9'};
    // Disconnect mid-upload aborts and returns to Idle.
    assert(!ota.onControlWrite(start9, 5, true, backend, err));
    assert(!ota.onDataWrite(payload, 4, true, backend, err));
    ota.onDisconnect(backend);
    assert(ota.status().state == OtaState::Idle && backend.aborts == 1 && !backend.active);
    // Unauthorized data write during a session aborts it.
    assert(!ota.onControlWrite(start9, 5, true, backend, err));
    assert(ota.onDataWrite(payload, 4, false, backend, err) && err == OtaError::WriteFailed);
    assert(ota.status().state == OtaState::Idle && backend.aborts == 2);
    // Retry succeeds after the aborted attempts.
    assert(!ota.onControlWrite(start9, 5, true, backend, err));
    assert(!ota.onDataWrite(payload, 9, true, backend, err));
    const uint8_t completeOk[5] = {0x02, 0x26, 0x39, 0xF4, 0xCB};
    assert(!ota.onControlWrite(completeOk, 5, true, backend, err));
    assert(ota.status().state == OtaState::RebootPending);
}

int main() {
    const uint8_t check[] = {'1','2','3','4','5','6','7','8','9'};
    assert(OtaSession::updateCrc32(0, check, sizeof(check)) == 0xCBF43926u);
    otaProtocolAndAuth();
    disconnectAbortsAndRetries();
    std::puts("OTA PASS");
}
