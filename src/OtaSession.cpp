#include "OtaSession.h"

namespace keyboardble {
namespace {
uint32_t readLe32(const uint8_t *p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
} // namespace

uint32_t OtaSession::updateCrc32(uint32_t crc, const uint8_t *data, size_t length) {
    uint32_t state = ~crc;
    for (size_t i = 0; i < length; ++i) {
        state ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            state = (state >> 1) ^ (0xEDB88320u & (-(state & 1u)));
        }
    }
    return ~state;
}

bool OtaSession::fail(OtaError err, OtaBackend *backendToAbort, OtaError &outError) {
    if (backendToAbort && _status.state == OtaState::Receiving) {
        backendToAbort->abort();
    }
    _status.state = OtaState::Idle;
    _status.lastError = err;
    ++_status.failures;
    outError = err;
    return true;
}

bool OtaSession::onControlWrite(const uint8_t *data, size_t length, bool authorized,
                                OtaBackend &backend, OtaError &outError) {
    outError = OtaError::None;
    if (!authorized) return fail(OtaError::BeginFailed, &backend, outError);
    if (!data || length < 1) return fail(OtaError::Unknown, &backend, outError);

    const uint8_t cmd = data[0];
    if (cmd == 0x01) { // START: [0x01, size_le32]
        if (length < 5) return fail(OtaError::BeginFailed, &backend, outError);
        if (_status.state == OtaState::Receiving) {
            backend.abort();
            _status.state = OtaState::Idle;
        }
        const uint32_t size = readLe32(data + 1);
        const uint32_t maxSize = backend.maxFirmwareSize();
        if (size == 0 || (maxSize > 0 && size > maxSize)) {
            return fail(OtaError::PartitionFull, nullptr, outError);
        }
        if (!backend.begin(size)) {
            return fail(OtaError::BeginFailed, nullptr, outError);
        }
        _status.state = OtaState::Receiving;
        _status.lastError = OtaError::None;
        _status.totalBytes = size;
        _status.receivedBytes = 0;
        _status.crc32 = 0;
        ++_status.sessions;
        return false;
    }

    if (cmd == 0x02) { // COMPLETE: [0x02, crc_le32]
        if (_status.state != OtaState::Receiving || length < 5) {
            return fail(OtaError::EndFailed, &backend, outError);
        }
        const uint32_t expectedCrc = readLe32(data + 1);
        if (_status.receivedBytes != _status.totalBytes || _status.crc32 != expectedCrc) {
            return fail(OtaError::CrcMismatch, &backend, outError);
        }
        if (!backend.end()) {
            return fail(OtaError::EndFailed, nullptr, outError);
        }
        _status.state = OtaState::RebootPending;
        _status.lastError = OtaError::None;
        return false;
    }

    if (cmd == 0x03) { // ABORT
        if (_status.state == OtaState::Receiving) {
            backend.abort();
        }
        _status.state = OtaState::Idle;
        return false;
    }

    return fail(OtaError::Unknown, &backend, outError);
}

bool OtaSession::onDataWrite(const uint8_t *data, size_t length, bool authorized,
                             OtaBackend &backend, OtaError &outError) {
    outError = OtaError::None;
    if (!authorized) return fail(OtaError::WriteFailed, &backend, outError);
    if (_status.state != OtaState::Receiving || !data || length == 0) {
        return false;
    }
    if (_status.receivedBytes + length > _status.totalBytes) {
        return fail(OtaError::PartitionFull, &backend, outError);
    }
    if (!backend.write(data, length)) {
        return fail(OtaError::WriteFailed, &backend, outError);
    }
    _status.crc32 = updateCrc32(_status.crc32, data, length);
    _status.receivedBytes += uint32_t(length);
    return false;
}

void OtaSession::onDisconnect(OtaBackend &backend) {
    if (_status.state == OtaState::Receiving) {
        backend.abort();
        _status.state = OtaState::Idle;
    }
}

} // namespace keyboardble
