// Portable Simple BLE OTA protocol state machine (kodira/simple-ble-ota compatible).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace keyboardble {

enum class OtaState : uint8_t { Idle, Receiving, RebootPending };

enum class OtaError : uint8_t {
    None          = 0x00,
    CrcMismatch   = 0x01,
    PartitionFull = 0x02,
    BeginFailed   = 0x03,
    WriteFailed   = 0x04,
    EndFailed     = 0x05,
    Unknown       = 0xFF
};

struct OtaStatus {
    OtaState state = OtaState::Idle;
    OtaError lastError = OtaError::None;
    uint32_t totalBytes = 0;
    uint32_t receivedBytes = 0;
    uint32_t crc32 = 0;
    uint32_t sessions = 0;
    uint32_t failures = 0;
    bool active() const { return state == OtaState::Receiving || state == OtaState::RebootPending; }
};

class OtaBackend {
public:
    virtual ~OtaBackend() = default;
    virtual uint32_t maxFirmwareSize() const = 0;
    virtual bool begin(uint32_t size) = 0;
    virtual bool write(const uint8_t *data, size_t length) = 0;
    virtual bool end() = 0;
    virtual void abort() = 0;
};

class OtaSession {
public:
    static constexpr const char *ServiceUuid = "487d0950-41b9-4c57-ad09-a46ac47e2150";
    static constexpr const char *ControlUuid = "487d0950-41b9-4c57-ad09-a46ac47e2151";
    static constexpr const char *DataUuid    = "487d0950-41b9-4c57-ad09-a46ac47e2152";

    static uint32_t updateCrc32(uint32_t crc, const uint8_t *data, size_t length);

    // Returns true if an error notification [0xFF, error] should be sent to the client.
    bool onControlWrite(const uint8_t *data, size_t length, bool authorized,
                        OtaBackend &backend, OtaError &outError);
    bool onDataWrite(const uint8_t *data, size_t length, bool authorized,
                     OtaBackend &backend, OtaError &outError);
    void onDisconnect(OtaBackend &backend);

    const OtaStatus &status() const { return _status; }

private:
    bool fail(OtaError err, OtaBackend *backendToAbort, OtaError &outError);
    OtaStatus _status;
};

} // namespace keyboardble
