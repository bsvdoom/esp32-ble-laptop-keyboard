"""Generate firmware identity from the current working sources and pinned config."""
Import("env")

import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

root = Path(env.subst("$PROJECT_DIR"))
build = Path(env.subst("$BUILD_DIR"))

# Persisted, fail-closed patch of the pinned library. Never rely on a hand-edited cache.
nimble = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env["PIOENV"] / "NimBLE-Arduino"
patch = root / "scripts/nimble-read-guard.patch"
expected = json.loads((root / "scripts/nimble-read-guard.json").read_text())
revision = subprocess.check_output(["git", "-C", str(nimble), "rev-parse", "HEAD"], text=True).strip()
if revision != "5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb":
    raise RuntimeError("Battery read guard requires pinned NimBLE-Arduino 2.5.1")
actual = {name: hashlib.sha256((nimble / name).read_bytes()).hexdigest() for name in expected}
if all(actual[name] == values["before"] for name, values in expected.items()):
    subprocess.run(["git", "-C", str(nimble), "apply", "--check", str(patch)], check=True)
    subprocess.run(["git", "-C", str(nimble), "apply", str(patch)], check=True)
elif not all(actual[name] == values["after"] for name, values in expected.items()):
    raise RuntimeError("Unexpected NimBLE read-guard inputs; preserving cache edits")
if not all(hashlib.sha256((nimble / name).read_bytes()).hexdigest() == values["after"]
           for name, values in expected.items()):
    raise RuntimeError("NimBLE read-guard patch verification failed")
patch_sha = hashlib.sha256(patch.read_bytes()).hexdigest()

def git(*args):
    result = subprocess.run(["git", "-C", str(root), *args], capture_output=True, text=True)
    return result.stdout.strip() if result.returncode == 0 else None

head = git("rev-parse", "--short=12", "HEAD") or "no-git"
status = git("status", "--porcelain", "--untracked-files=normal", "--ignore-submodules=none")
state = "unknown" if status is None else "dirty" if status else "clean"
# Content fingerprint excludes logs/docs/generated files, includes actual local sources.
compile_fixture = "KEYBOARD_BATTERY_COMPILE_CHECK" in str(env.GetProjectOption("build_flags", []))
adc_test_configuration = "KEYBOARD_ADC_TEST_CONFIGURATION" in str(env.GetProjectOption("build_flags", []))
paths = [root / name for name in ("platformio.ini", "custom.csv", "requirements-build.txt")]
if compile_fixture:
    paths.append(root / "tools/battery_compile_fixture.h")
if adc_test_configuration:
    paths.append(root / "tools/adc_test_configuration.h")
for folder in ("src", "include", "scripts", "lib/CD74HC4067"):
    base = root / folder
    if base.exists():
        paths.extend(p for p in base.rglob("*") if p.is_file()
                     and ".git" not in p.parts and "__pycache__" not in p.parts)
digest = hashlib.sha256()
for path in sorted(paths):
    digest.update(str(path.relative_to(root)).encode() + b"\0" + path.read_bytes() + b"\0")
identity = head + "-" + state + "-src" + digest.hexdigest()[:12]
if compile_fixture:
    identity += "-BATTERY-TEST-ONLY"
if adc_test_configuration:
    identity += "-ADC-TEST-CONFIGURATION"
generated = build / "generated"
generated.mkdir(parents=True, exist_ok=True)
timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
header = '#pragma once\n#define KEYBOARD_BUILD_ID ' + json.dumps(identity) + '\n'
header += '#define KEYBOARD_BUILD_UTC ' + json.dumps(timestamp) + '\n'
(generated / "KeyboardBuildInfo.h").write_text(header)
(generated / "build-info.json").write_text(json.dumps({
    "build_id": identity, "built_utc": timestamp, "ble_stack": "NimBLE-Arduino 2.5.1",
    "nimble_commit": "5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb",
    "nimble_read_guard_sha256": patch_sha,
    "battery_compile_fixture": compile_fixture,
    "adc_test_configuration": adc_test_configuration
}, indent=2) + "\n")
env.Prepend(CPPPATH=[str(generated)])
print("Keyboard build:", identity, timestamp)
