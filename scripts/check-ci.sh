#!/usr/bin/env bash
set -euo pipefail
project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd -- "$project_root"

if [[ -x ./bin/pio ]]; then
  pio_cmd=(./bin/pio)
elif [[ -x "${HOME}/.platformio/penv/bin/pio" ]]; then
  pio_cmd=("${HOME}/.platformio/penv/bin/pio")
else
  pio_cmd=(pio)
fi

if [[ -d checks ]]; then
  # Touchpad hostchecks, one C3 board with two opt-in configurations.
  for check in checks/run-ps2.sh checks/run-device.sh checks/run-relative.sh \
               checks/run-mouse.sh checks/run-ble.sh checks/run-integration.sh \
               checks/run-battery.sh; do
    "$check"
  done
  "${pio_cmd[@]}" run -e esp32c3 -e esp32c3-battery-example
elif [[ -d tools ]]; then
  # Keyboard hostchecks and C3 firmware build.
  check_dir="$project_root/.pio/checks"
  mkdir -p "$check_dir"
  cxx_flags=(-std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iinclude)
  run_check() {
    local name="$1"
    shift
    g++ "${cxx_flags[@]}" "$@" -o "$check_dir/$name"
    ASAN_OPTIONS=detect_leaks=0 "$check_dir/$name"
  }
  run_check check_led         tools/check_led.cpp
  run_check check_hid         tools/check_hid.cpp         src/KeyboardState.cpp
  run_check check_state       tools/check_state.cpp       src/KeyboardState.cpp
  run_check check_session     tools/check_session.cpp     src/KeyboardSession.cpp src/KeyboardState.cpp
  run_check check_battery     tools/check_battery.cpp     src/BatteryState.cpp
  run_check check_adc_battery tools/check_adc_battery.cpp src/BatteryAdcMath.cpp
  run_check check_ota         tools/check_ota.cpp         src/OtaSession.cpp
  "${pio_cmd[@]}" run -e esp32c3
else
  echo "Unknown project layout in $project_root" >&2
  exit 1
fi
