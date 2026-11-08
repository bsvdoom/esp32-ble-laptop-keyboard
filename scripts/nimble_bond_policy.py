"""Compile scoped bond-preservation patches; never edit installed packages.

Original NimBLEServer.cpp / notices: Apache-2.0, h2zero/NimBLE-Arduino.
Local changes: refuse repeat-pairing and automatic recovery-erasure of NVS;
optional own Battery Level access hook for explicit ATT errors (step 8).
Sources/notices: NimBLE 2.5.1 and pinned Arduino HAL (Apache-2.0).
"""
from hashlib import sha256
from pathlib import Path

Import("env")

SOURCE_SHA = {
    "NimBLEServer.cpp": "af4335cf6b9e5ca3be63770307736fe50d29b53529c0dd3579f7ed515b553895",
    "NimBLEDevice.cpp": "b882da0ac028295725a6081e5e56f65c32b838bdaec3ae6ecde4ca89d17f6bed",
    "esp32-hal-misc.c": "0f6057685866ce45208f1d1e5b9e81164aca1d580678b7be8c97227dacc970d9",
}


def preserve_bonds(build_env, node):
    path = Path(node.srcnode().get_abspath())
    if "NimBLE-Arduino" not in path.parts and "framework-arduinoespressif32" not in path.parts:
        return node
    original = path.read_bytes()
    if sha256(original).hexdigest() != SOURCE_SHA[path.name]:
        raise RuntimeError(f"{path.name} source hash mismatch: review bond policy before building")
    text = original.decode()
    if path.name == "NimBLEServer.cpp":
        # Upstream REPEAT_PAIRING kept: a host that forgot the device must be
        # able to pair again (old bond for that same peer is replaced).
        patched = text
        # NimBLE 2.5.1's void onRead cannot express an unavailable measurement.
        # Own hook handles only the battery value handle, returns -1 for others.
        signature = "int NimBLEServer::handleGattEvent("
        assert text.count(signature) == 1
        patched = patched.replace(signature, """// Local optional Battery Level access policy; original notices preserved.
extern "C" int touchpad_battery_read(uint16_t, uint8_t*) __attribute__((weak));

""" + signature, 1)
        marker = "        case BLE_GATT_ACCESS_OP_READ_CHR: {\n"
        assert patched.count(marker) == 1
        patched = patched.replace(marker, marker + """            if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR && touchpad_battery_read) {
                uint8_t batteryLevel;
                const int result = touchpad_battery_read(attrHandle, &batteryLevel);
                if (result >= 0) {
                    if (result != 0) return result;
                    return os_mbuf_append(ctxt->om, &batteryLevel, 1) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
                }
            }
""", 1)
    elif path.name == "NimBLEDevice.cpp":
        start = text.index("        if (err == ESP_ERR_NVS_NO_FREE_PAGES")
        end = text.index("        if (err != ESP_OK)", start)
        patched = text[:start] + "        // Local policy: never erase NVS/bonds on startup error.\n" + text[end:]
    else:
        start = text.index("  if (err == ESP_ERR_NVS_NO_FREE_PAGES")
        end = text.index("  if (err) {", start)
        patched = text[:start] + "  // Local policy: preserve NVS/bonds; BLE reports startup error.\n" + text[end:]
    destination = Path(build_env.subst("$BUILD_DIR")) / "bond-policy" / path.name
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text() != patched:
        destination.write_text(patched)
    return build_env.File(str(destination))


env.AddBuildMiddleware(preserve_bonds, "*NimBLEServer.cpp")
env.AddBuildMiddleware(preserve_bonds, "*NimBLEDevice.cpp")
env.AddBuildMiddleware(preserve_bonds, "*esp32-hal-misc.c")
