"""Refuse USB uploads that could hit the host's esp32-bt-adapter (BlueZ hci0).

The host's only BLE controller is an ESP32-C3 CH340 board attached via btattach.
Both boards are CH340s without serial numbers, so auto-detection may pick the
adapter and overwrite its firmware. Uploads therefore require an explicit
--upload-port that is neither the adapter's known path nor a tty with an hci child.
"""
Import("env")

import os
from pathlib import Path

ADAPTER_BY_PATH = "/dev/serial/by-path/pci-0000:00:14.0-usb-0:3.1.2.1:1.0-port0"


def _is_adapter(port):
    real = os.path.realpath(port)
    if os.path.exists(ADAPTER_BY_PATH) and real == os.path.realpath(ADAPTER_BY_PATH):
        return "it is the esp32-bt-adapter by-path port"
    tty = Path("/sys/class/tty") / os.path.basename(real)
    if tty.is_dir() and any(child.name.startswith("hci") for child in tty.iterdir()):
        return "it has an attached hci device (btattach)"
    return None


def guard_upload(source, target, env):
    port = env.subst("$UPLOAD_PORT").strip()
    if not port:
        raise SystemExit("upload_guard: refusing auto-detected port; pass --upload-port "
                         "<target tty> (never the esp32-bt-adapter)")
    reason = _is_adapter(port)
    if reason:
        raise SystemExit("upload_guard: refusing %s: %s" % (port, reason))
    print("upload_guard: upload port %s -> %s (not the BT adapter)" % (port, os.path.realpath(port)))


env.AddPreAction("upload", guard_upload)
