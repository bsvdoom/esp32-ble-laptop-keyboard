#pragma once
#include <stdint.h>

enum class KeyboardSendResult { Disconnected, NotReady, StaleSession, LocalAccepted, LocalRejected };

struct KeyboardLinkSnapshot {
  uint64_t generation = 0;
  uint16_t handle = UINT16_MAX;
  bool connected = false, encrypted = false, authenticated = false, bonded = false, subscribed = false;
  bool ready() const { return connected && encrypted && subscribed; }
};

// Adapter holds its mutex for these scalar operations and snapshot copies only.
// Shared by the firmware callbacks and native lifecycle traces.
class KeyboardLinkState {
public:
  KeyboardLinkSnapshot snapshot() const { return link_; }
  void connect(uint16_t handle, bool encrypted, bool authenticated, bool bonded) {
    const uint64_t generation = link_.generation + 1;
    link_ = KeyboardLinkSnapshot{};
    link_.generation = generation;
    link_.handle = handle;
    link_.connected = true;
    link_.encrypted = encrypted;
    link_.authenticated = authenticated;
    link_.bonded = bonded;
  }
  void disconnect(uint16_t handle) {
    if (!link_.connected || link_.handle != handle) return;
    const uint64_t generation = link_.generation + 1;
    link_ = KeyboardLinkSnapshot{};
    link_.generation = generation;
  }
  void security(uint16_t handle, bool encrypted, bool authenticated, bool bonded) {
    if (!link_.connected || link_.handle != handle) return;
    const bool wasReady = link_.ready();
    link_.encrypted = encrypted;
    link_.authenticated = authenticated;
    link_.bonded = bonded;
    if (wasReady != link_.ready()) ++link_.generation;
  }
  void subscription(uint16_t handle, bool notify, bool encrypted, bool authenticated, bool bonded) {
    if (!link_.connected || link_.handle != handle) return;
    const bool wasReady = link_.ready();
    link_.subscribed = notify;
    link_.encrypted = encrypted;
    link_.authenticated = authenticated;
    link_.bonded = bonded;
    if (wasReady != link_.ready()) ++link_.generation;
  }
private:
  KeyboardLinkSnapshot link_;
};
