#!/usr/bin/env bash
# Pair, trust and connect the BLE HID device ("ESP32 Keyboard" or "C3 Laptop Touchpad")
# via BlueZ, then verify that a Linux input device was created.
#
#   ./scripts/pair-ble.sh [NAME|MAC] [--adapter hciX] [--scan SEC] [--remove]
#
#   NAME|MAC   default: auto-detected ("ESP32 Keyboard" or "C3 Laptop Touchpad")
#   --adapter  controller to use (default: hci0 or esp32-bt-adapter)
#   --scan     scan timeout in seconds (default 20)
#   --remove   forget the device first (fresh bonding)
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
if [[ -f "$ROOT/include/KeyboardHid.h" ]]; then
    TARGET="ESP32 Keyboard"
else
    TARGET="C3 Laptop Touchpad"
fi
ADAPTER=""
SCAN=20
REMOVE=0

log() { printf '[pair-ble] %s\n' "$*" >&2; }
die() { log "HIBA: $*"; exit 1; }

while (($#)); do
    case $1 in
        --adapter) ADAPTER=$2; shift ;;
        --scan) SCAN=$2; shift ;;
        --remove) REMOVE=1 ;;
        -h|--help) sed -n '2,/^set -euo/p' "$0" | sed '$d;s/^# \{0,1\}//'; exit 0 ;;
        -*) die "ismeretlen opció: $1" ;;
        *) TARGET=$1 ;;
    esac
    shift
done

if command -v rfkill >/dev/null 2>&1 && rfkill list bluetooth 2>/dev/null | grep -q 'Soft blocked: yes'; then
    log "rfkill unblock bluetooth"
    rfkill unblock bluetooth || true
fi

if [[ -x /home/adam/Projects/esp32-bt-adapter/scripts/attach-hci.sh ]]; then
    if ! hciconfig 2>/dev/null | grep -q 'UP RUNNING'; then
        /home/adam/Projects/esp32-bt-adapter/scripts/attach-hci.sh >/dev/null
    fi
fi

systemctl is-active --quiet bluetooth || die "bluetooth.service nem fut"

if [[ -z $ADAPTER && -s /home/adam/Projects/esp32-bt-adapter/.adapter-port ]]; then
    tty=$(basename "$(readlink -f "$(</home/adam/Projects/esp32-bt-adapter/.adapter-port)")")
    for d in /sys/class/tty/"$tty"/hci*; do [[ -e $d ]] && ADAPTER=$(basename "$d"); done
fi
CTRL_ADDR=""
if [[ -n $ADAPTER ]]; then
    CTRL_ADDR=$(cat /sys/class/bluetooth/"$ADAPTER"/address 2>/dev/null || true)
    [[ -n $CTRL_ADDR ]] || CTRL_ADDR=$(hciconfig "$ADAPTER" 2>/dev/null | awk '/BD Address/{print $3}')
    [[ -n $CTRL_ADDR ]] || die "nincs ilyen adapter: $ADAPTER"
    log "adapter: $ADAPTER ($CTRL_ADDR)"
fi

btctl() {
    local cmds=()
    [[ -n $CTRL_ADDR ]] && cmds+=("select $CTRL_ADDR")
    cmds+=("$@")
    printf '%s\n' "${cmds[@]}" | bluetoothctl 2>&1 | sed 's/\x1b\[[0-9;]*m//g; s/\r//g'
}

btctl_t() {
    local t=$1; shift
    if [[ -n $CTRL_ADDR ]]; then
        { [[ -n $CTRL_ADDR ]] && echo "select $CTRL_ADDR"; echo "$*"; sleep "$t"; } \
            | bluetoothctl 2>&1 | sed 's/\x1b\[[0-9;]*m//g; s/\r//g'
    else
        bluetoothctl --timeout "$t" "$@" 2>&1 | sed 's/\x1b\[[0-9;]*m//g; s/\r//g'
    fi
}

find_addr() {
    if [[ $TARGET =~ ^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$ ]]; then echo "${TARGET^^}"; return; fi
    btctl "devices" | awk -v n="$TARGET" '$1=="Device" { a=$2; $1=$2=""; sub(/^  /,""); if ($0==n) print a }' | tail -1
}

ADDR=$(find_addr || true)
if ((REMOVE)) && [[ -n $ADDR ]]; then
    log "eltávolítás: $ADDR"
    btctl "remove $ADDR" >/dev/null
    ADDR=""
    sleep 1
fi

if [[ -z $ADDR ]] || ! btctl "info $ADDR" | grep -q 'Paired: yes'; then
    log "keresés: \"$TARGET\" (max ${SCAN}s)..."
    btctl_t "$SCAN" "scan le" >/dev/null &
    SCANPID=$!
    for _ in $(seq 1 "$SCAN"); do
        sleep 1
        ADDR=$(find_addr || true)
        [[ -n $ADDR ]] && break
    done
    kill "$SCANPID" 2>/dev/null || true
    wait "$SCANPID" 2>/dev/null || true
    [[ -n $ADDR ]] || die "nem található: $TARGET"
    log "megtalálva: $ADDR"

    log "párosítás (Just Works)..."
    out=$( { [[ -n $CTRL_ADDR ]] && echo "select $CTRL_ADDR"
             echo "agent NoInputNoOutput"; echo "default-agent"
             echo "scan le"; sleep 3
             echo "pair $ADDR"; sleep 12
             echo "scan off"; sleep 1; } | bluetoothctl 2>&1 | sed 's/\x1b\[[0-9;]*m//g; s/\r//g')
    if ! grep -qE 'Pairing successful|AlreadyExists|Paired: yes' <<<"$out" \
        && ! btctl "info $ADDR" | grep -q 'Paired: yes'; then
        echo "$out" | grep -iE 'fail|error|pair' >&2 || true
        die "párosítás sikertelen"
    fi
    log "párosítva"
else
    log "már párosítva: $ADDR"
fi

btctl "trust $ADDR" >/dev/null
log "trusted"

if ! btctl "info $ADDR" | grep -q 'Connected: yes'; then
    log "csatlakozás..."
    { [[ -n $CTRL_ADDR ]] && echo "select $CTRL_ADDR"; echo "connect $ADDR"; sleep 8; } | bluetoothctl >/dev/null 2>&1 || true
fi

INFO=$(btctl "info $ADDR")
grep -q 'Connected: yes' <<<"$INFO" || die "nem csatlakozott"
grep -E 'Name:|Paired:|Bonded:|Trusted:|Connected:|Battery Percentage:' <<<"$INFO" | sed 's/^\s*/  /' >&2

log "input eszköz ellenőrzése..."
target_lower="${TARGET,,}"
for _ in $(seq 1 20); do
    blk=$(awk -v RS= -v a="${ADDR,,}" -v t="$target_lower" 'tolower($0) ~ ("uniq=" a) || index(tolower($0), t) > 0' /proc/bus/input/devices || true)
    [[ -n $blk ]] && break
    sleep 0.5
done
[[ -n ${blk:-} ]] || die "nem jelent meg input eszköz a /proc/bus/input/devices-ban"
echo "$blk" | grep -E '^(N|U|H):' | sed 's/^/  /' >&2
log "OK: $TARGET ($ADDR) csatlakozva, input eszköz: $(grep -oE '(event|mouse|kbd)[0-9]*' <<<"$blk" | tr '\n' ' ')"
