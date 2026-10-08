#!/bin/bash -ex

PORT="${1:?Usage: recv.sh PORT [LOGFILE]}"
SPEED="115200"

LOGFILE="${2:-LOG_$(date +%y%m%d%H%M).BFL}"

stty -F "$PORT" "$SPEED" raw -echo
cat < "$PORT" > "$LOGFILE"

