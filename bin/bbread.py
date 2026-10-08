#!/usr/bin/env python3
"""Raw 115200-baud capture plus UTC receive timestamps; does not upload firmware."""
import argparse
import signal
from datetime import datetime, timezone
from pathlib import Path
import serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="verified serial port, e.g. /dev/ttyUSB1 or COM3")
    parser.add_argument("log", type=Path, help="raw log filename; .utc.txt sidecar is also saved")
    args = parser.parse_args()
    stop = False

    def interrupt(signum, frame):
        nonlocal stop
        stop = True

    signal.signal(signal.SIGINT, interrupt)
    with serial.Serial(port=None, baudrate=115200, timeout=0.2) as connection:
        # Set before open; board/driver-specific line glitches remain unverified.
        connection.dtr = False
        connection.rts = False
        connection.port = args.port
        with args.log.open("xb") as raw, Path(str(args.log) + ".utc.txt").open("x") as timed:
            connection.open()
            print("Capture:", args.port, "115200 ->", args.log, "(Ctrl+C stops)")
            while not stop:
                chunk = connection.read(256)
                if not chunk:
                    continue
                raw.write(chunk)
                timed.write(datetime.now(timezone.utc).isoformat() + " " + repr(chunk) + "\n")
                raw.flush()
                timed.flush()


if __name__ == "__main__":
    main()
