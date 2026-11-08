#!/usr/bin/env python3
"""
Simple BLE OTA Client for ESP32 Laptop Keyboard / C3 Laptop Touchpad
(kodira/simple-ble-ota compatible)

Usage:
  ./scripts/simple-ble-ota-client.py [firmware.bin] [device_name_or_mac]

Defaults:
  firmware.bin       -> .pio/build/esp32c3/firmware.bin
  device_name_or_mac -> auto-detected ("ESP32 Keyboard" or "C3 Laptop Touchpad")

Requirements:
  pip install bleak
  Linux with BlueZ BLE adapter (hci0)
"""

import asyncio
import subprocess
import sys
import time
import zlib
from pathlib import Path

try:
    from bleak import BleakClient, BleakScanner
except ImportError:
    print("Missing dependency: bleak (install with: pip install bleak)")
    sys.exit(1)

SIMPLE_OTA_SERVICE_UUID = "487d0950-41b9-4c57-ad09-a46ac47e2150"
SIMPLE_OTA_CTRL_UUID    = "487d0950-41b9-4c57-ad09-a46ac47e2151"
SIMPLE_OTA_DATA_UUID    = "487d0950-41b9-4c57-ad09-a46ac47e2152"

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_DEVICE_NAME = (
    "ESP32 Keyboard"
    if (PROJECT_ROOT / "include/KeyboardHid.h").is_file()
    else "C3 Laptop Touchpad"
)
DEFAULT_FIRMWARE_PATH = ".pio/build/esp32c3/firmware.bin"
CHUNK_SIZE = 244
BATCH_SIZE = 10


def find_connected_bluez_address(target_name: str):
    """If the BLE HID device is already connected in BlueZ, it won't advertise; query bluetoothctl."""
    try:
        out = subprocess.check_output(
            ["bluetoothctl", "devices"], text=True, stderr=subprocess.DEVNULL, timeout=3
        )
        for line in out.splitlines():
            # Format: "Device XX:XX:XX:XX:XX:XX <Device Name>"
            parts = line.strip().split(" ", 2)
            if len(parts) == 3 and parts[0] == "Device" and parts[2] == target_name:
                return parts[1]
    except Exception:
        pass
    return None


class SimpleOtaClient:
    def __init__(self):
        self.client = None
        self.is_connected = False
        self.error_received = None

    def notification_handler(self, _sender, data: bytearray):
        if len(data) >= 2 and data[0] == 0xFF:
            error_code = data[1]
            error_messages = {
                0x01: "CRC mismatch - firmware corrupted during transfer",
                0x02: "Partition full - insufficient space for firmware",
                0x03: "OTA begin failed (or link not encrypted/bonded)",
                0x04: "OTA write failed - error writing firmware data",
                0x05: "OTA end failed - could not finalize update",
                0xFF: "Unknown error occurred",
            }
            self.error_received = error_messages.get(
                error_code, f"Unknown error code: 0x{error_code:02X}"
            )
            print(f"\nESP32 Error: {self.error_received}")

    async def resolve_and_connect(self, target: str, timeout: float = 30.0):
        address = None
        if ":" in target and len(target) == 17:
            address = target
        else:
            address = find_connected_bluez_address(target)
            if address:
                print(f"Found paired/connected BlueZ device '{target}' at {address}")

        if not address:
            print(f"Scanning for BLE device '{target}'...")
            start_time = time.time()
            while time.time() - start_time < timeout:
                devices = await BleakScanner.discover(timeout=2.0)
                for d in devices:
                    if d.name == target:
                        address = d.address
                        break
                if address:
                    break
                print(".", end="", flush=True)
            print()

        if not address:
            raise RuntimeError(f"Device '{target}' not found within {timeout:.0f}s")

        print(f"Connecting to {address}...")
        self.client = BleakClient(address)
        await self.client.connect()
        self.is_connected = True
        print("Connected.")

        await self.client.start_notify(SIMPLE_OTA_CTRL_UUID, self.notification_handler)

    async def upload_firmware(self, firmware_path: Path):
        data = firmware_path.read_bytes()
        file_size = len(data)
        crc = zlib.crc32(data) & 0xFFFFFFFF
        total_chunks = (file_size + CHUNK_SIZE - 1) // CHUNK_SIZE

        print(f"Firmware: {firmware_path} ({file_size} bytes, CRC32: 0x{crc:08X})")
        start_cmd = bytes([
            0x01,
            file_size & 0xFF,
            (file_size >> 8) & 0xFF,
            (file_size >> 16) & 0xFF,
            (file_size >> 24) & 0xFF,
        ])
        await self.client.write_gatt_char(SIMPLE_OTA_CTRL_UUID, start_cmd, response=True)
        await asyncio.sleep(0.05)
        if self.error_received:
            raise RuntimeError(self.error_received)

        start_time = time.time()
        sent_bytes = 0
        for chunk_idx, offset in enumerate(range(0, file_size, CHUNK_SIZE), start=1):
            chunk = data[offset : offset + CHUNK_SIZE]
            with_response = (chunk_idx % BATCH_SIZE == 0) or (chunk_idx == total_chunks)
            await self.client.write_gatt_char(
                SIMPLE_OTA_DATA_UUID, chunk, response=with_response
            )
            if self.error_received:
                raise RuntimeError(self.error_received)
            sent_bytes += len(chunk)
            elapsed = max(time.time() - start_time, 1e-3)
            rate_kbs = (sent_bytes / 1024.0) / elapsed
            pct = (sent_bytes * 100.0) / file_size
            print(
                f"\rProgress: {pct:5.1f}% ({sent_bytes}/{file_size} B) @ {rate_kbs:5.1f} KB/s",
                end="",
                flush=True,
            )
        print()

        complete_cmd = bytes([
            0x02,
            crc & 0xFF,
            (crc >> 8) & 0xFF,
            (crc >> 16) & 0xFF,
            (crc >> 24) & 0xFF,
        ])
        try:
            await self.client.write_gatt_char(SIMPLE_OTA_CTRL_UUID, complete_cmd, response=True)
            await asyncio.sleep(0.5)
        except Exception:
            if self.error_received:
                raise RuntimeError(self.error_received)
        if self.error_received:
            raise RuntimeError(self.error_received)
        print("OTA completed and verified! Device is rebooting into new firmware.")

    async def disconnect(self):
        if self.client and self.is_connected:
            try:
                await self.client.disconnect()
            except Exception:
                pass
            self.is_connected = False


async def main():
    fw_arg = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_FIRMWARE_PATH
    target_arg = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_DEVICE_NAME
    fw_path = Path(fw_arg)
    if not fw_path.is_file():
        print(f"Firmware file not found: {fw_path}")
        sys.exit(1)

    client = SimpleOtaClient()
    try:
        await client.resolve_and_connect(target_arg)
        await client.upload_firmware(fw_path)
    finally:
        await client.disconnect()


if __name__ == "__main__":
    asyncio.run(main())
