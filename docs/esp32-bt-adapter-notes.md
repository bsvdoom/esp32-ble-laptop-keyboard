# Notes for the agent working on the C3 Laptop Touchpad

The Linux host no longer has a built-in Bluetooth adapter. The only BLE
controller is **esp32-bt-adapter**, an ESP32-C3 dev board (CH340 USB-UART)
running controller-only firmware, attached to BlueZ as `hci0` via `btattach`.
Project: `/home/adam/Projects/esp32-bt-adapter` (README.md has the details).

## 1. Serial port collision (dangerous)

- The adapter is on **`/dev/ttyUSB0`** (CH340, `/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0`).
- The touchpad's `platformio.ini` hard-codes `upload_port = /dev/ttyUSB0` and
  `monitor_port = /dev/ttyUSB0`. **`pio run -t upload` would overwrite the
  adapter firmware.** `pio device monitor` on that port would also corrupt the H4 stream.
- Both boards are CH340 boards with no USB serial number. Their by-id names are
  identical and collide, so tell them apart by port path or by probing:
  - `ls /sys/class/tty/ttyUSB*/hci*` shows which tty is the adapter
    (it has an `hciX` child while attached).
  - `python3 /home/adam/Projects/esp32-bt-adapter/scripts/hci_probe.py /dev/ttyUSBx`
    exits with 0 only on the adapter (sends HCI_Reset, does not reset other boards).
  - The adapter's stable path is in `/home/adam/Projects/esp32-bt-adapter/.adapter-port`
    (`/dev/serial/by-path/pci-0000:00:14.0-usb-0:3.1.2.1:1.0-port0`).
- Never run `esptool` on the adapter port. esptool resets the board, and
  BlueZ loses `hci0`.
- If the touchpad is flashed over USB, use `--upload-port /dev/ttyUSB1` (or
  whichever is NOT the adapter), or a by-path name.

## 2. Adapter must be attached

- After a reboot or replug of the adapter, run
  `/home/adam/Projects/esp32-bt-adapter/scripts/attach-hci.sh` (idempotent).
  No systemd unit is installed yet.
- Check status with `scripts/status-hci.sh` (exit 0 = OK). `bluetooth.service` is
  started automatically when `hci0` appears.
- Passwordless sudo exists only for exact commands: `/usr/bin/btmon`,
  `/usr/bin/btmgmt`, `/usr/bin/hciconfig`, `/usr/bin/btattach`,
  `systemctl start|stop|restart bluetooth`, `pkill -x btattach`.
- `btmgmt` hangs forever with stdin `/dev/null`. Use `: | sudo -n /usr/bin/btmgmt ...`.

## 3. Host identity and bonding

- The host (central) address is now **`84:F7:03:A5:B2:FA`** (public). Bonds made
  with any previous host/adapter do not apply. The touchpad needs a new bond
  with this address, and it uses one bond slot on the touchpad.
- Pairing: BlueZ 5.85, LE Secure Connections, Just Works
  (`scripts/pair-touchpad.sh` in the adapter project does scan, pair, trust,
  connect and the input-device check).
- If a fresh bond is needed: `bluetoothctl remove <touchpad MAC>` on the host.
  The touchpad must allow re-pairing from the same peer (its bond policy keeps
  REPEAT_PAIRING).
- The adapter's BD_ADDR comes from eFuse, so it is stable across reflashes of
  the adapter.

## 4. Controller capabilities (measured from btmon at init)

- BLE 5 (HCI version 9), manufacturer 0x02E5 Espressif. LE only, no BR/EDR.
- Data Length Extension: max 251 octets TX/RX. ACL buffers: 12 x 251 B.
- LE 2M PHY and Coded PHY, LL Privacy, Connection Parameter Request procedure,
  LE Ping, Extended Advertising.
- Accept list: 12 entries. Resolving list: 10 entries.
- BlueZ requests ATT MTU 517, so 244 B OTA chunks fit (needs MTU >= 247).
- Host link is UART 921600 baud with no hardware flow control. That is
  ~90 KB/s raw per direction, enough for 30-40 KB/s OTA (OTA not yet measured).

## 5. Connection interval

- On attach the host's default LE connection interval request is set to
  **6..12 (7.5-15 ms)** via MGMT sysconfig (debugfs is blocked by Secure Boot
  lockdown). This is per boot of the host and is reapplied by `attach-hci.sh`.
- The touchpad's own L2CAP Connection Parameter Update request is normally
  accepted by the kernel. Verify with
  `sudo -n /usr/bin/btmon | grep -A6 'Connection Update Complete'` or
  `scripts/status-hci.sh -v`.
- Peripheral-side preferred parameters (PPCP) or update requests outside
  7.5-15 ms will override this. Keep the touchpad's requested interval in that range.

## 6. OTA through the adapter

- `bleak` (D-Bus/BlueZ backend) works through `hci0` (scan verified).
- `/home/adam/Projects/esp32-bt-adapter/.venv-ble` has bleak 3.1.0.
  `scripts/ota_bleak.py` speaks the same Simple BLE OTA protocol as the touchpad's
  `scripts/simple-ble-ota-client.py`, adds `--dry-run` and KB/s output, and uses the
  adapter by default.
- OTA requires an encrypted (bonded) link, error 0x03 otherwise. While the
  touchpad is connected as HID it does not advertise, so the clients look the
  address up in BlueZ (`bluetoothctl devices`).

## 7. Debugging tips

- `sudo -n /usr/bin/btmon` shows all HCI traffic (pairing, SMP, conn updates,
  disconnect reasons).
- If `hci0` disappears: `scripts/attach-hci.sh`. If it keeps failing:
  `scripts/attach-hci.sh --stop`, then `python3 scripts/hci_probe.py <port> --settle 1.5`.
- Disconnect reason 0x08 (supervision timeout) with the adapter still UP
  means an RF/range issue on the touchpad side. `hci0` vanishing means the
  adapter or USB side.
