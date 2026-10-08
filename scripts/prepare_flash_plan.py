"""Align Arduino's unchanged OTA initializer with our table; never open a port.

Pinned Arduino 2.0.17 / espressif32 6.13.0 append boot_app0 at a fixed E000.
The platform copies those pairs into UPLOADERFLAGS before this post script.
Correct both representations, then validate and record actual build artifacts.
"""
Import("env")

import hashlib
import importlib.util
import json
from pathlib import Path

framework = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32"))
spec = importlib.util.spec_from_file_location("keyboard_partition_parser",
                                           framework / "tools/gen_esp32part.py")
parser = importlib.util.module_from_spec(spec)
spec.loader.exec_module(parser)
parser.offset_part_table = 0x8000  # Same fixed table offset as the pinned Arduino builder.
parser.quiet = True
table = parser.PartitionTable.from_csv(Path(env.subst("$PARTITIONS_TABLE_CSV")).read_text())
table.verify()
ota = [p for p in table if p.type == parser.DATA_TYPE and p.subtype == 0]
if len(ota) != 1 or ota[0].size != 8192 or ota[0].offset % 4096:
    raise RuntimeError("Expected exactly one aligned 8 KiB OTA-data partition")
initializer = (framework / "tools/partitions/boot_app0.bin").resolve()
if initializer.stat().st_size != ota[0].size:
    raise RuntimeError("OTA initializer size differs from OTA-data partition")

images = list(env.get("FLASH_EXTRA_IMAGES", []))
matches = [i for i, (_, path) in enumerate(images)
           if Path(env.subst(str(path))).resolve() == initializer]
if len(matches) != 1:
    raise RuntimeError("Expected exactly one framework boot_app0 image")
i = matches[0]
old_offset, path = images[i]
images[i] = (hex(ota[0].offset), path)
flags = list(env.get("UPLOADERFLAGS", []))
matches = [i for i, value in enumerate(flags)
           if env.subst(str(value)) == str(initializer)]
if len(matches) != 1 or matches[0] == 0 or flags[matches[0] - 1] != old_offset:
    raise RuntimeError("Pinned esptool uploader no longer matches FLASH_EXTRA_IMAGES")
flags[matches[0] - 1] = hex(ota[0].offset)
env.Replace(FLASH_EXTRA_IMAGES=images, UPLOADERFLAGS=flags)
print("Keyboard flash plan: boot_app0", old_offset, "->", hex(ota[0].offset))


def record_plan(source, target, env):
    build = Path(env.subst("$BUILD_DIR"))
    firmware = Path(str(target[0]))
    elf = build / (env.subst("$PROGNAME") + ".elf")
    binary_table = parser.PartitionTable.from_binary((build / "partitions.bin").read_bytes())
    if binary_table.to_binary() != table.to_binary():
        raise RuntimeError("Built partition table differs from configured CSV")
    elf_sha = hashlib.sha256(elf.read_bytes()).digest()
    if firmware.read_bytes()[0xb0:0xd0] != elf_sha:
        raise RuntimeError("Firmware embedded ELF SHA256 differs from actual ELF")
    app_offset = int(env.subst("$ESP32_APP_OFFSET"), 0)
    app = [p for p in table if p.type == parser.APP_TYPE and p.offset == app_offset]
    if len(app) != 1 or app[0].subtype != 0x10:
        raise RuntimeError("Expected firmware at the OTA0 partition start")
    pairs = [(int(offset, 0), Path(env.subst(str(path))))
             for offset, path in env["FLASH_EXTRA_IMAGES"]]
    pairs.append((app_offset, firmware))
    expected = {"bootloader.bin": (0, 0x8000, "bootloader region"),
                "partitions.bin": (0x8000, 0x9000, "partition table"),
                "boot_app0.bin": (ota[0].offset, ota[0].offset + ota[0].size, ota[0].name),
                firmware.name: (app_offset, app_offset + app[0].size, app[0].name)}
    if len(pairs) != len(expected) or {p.name for _, p in pairs} != set(expected):
        raise RuntimeError("Unexpected flash images; review the artifact plan")
    records = []
    for offset, path in sorted(pairs):
        data = path.read_bytes()
        start, limit, name = expected[path.name]
        end = offset + len(data)
        erase_start, erase_end = offset // 4096 * 4096, (end + 4095) // 4096 * 4096
        if offset != start or not data or end > limit or erase_end > limit:
            raise RuntimeError("Image/write/erase range exceeds its intended region: " + path.name)
        if records and erase_start < records[-1]["erase_end_exclusive"]:
            raise RuntimeError("Flash erase sectors overlap between images")
        records.append(dict(file=path.name, source=str(path), size=len(data),
                            sha256=hashlib.sha256(data).hexdigest(), offset=offset,
                            end_exclusive=end, target=name, erase_start=erase_start,
                            erase_end_exclusive=erase_end))
    # Confirm the already assembled uploader uses these same corrected extra pairs.
    flags = list(env["UPLOADERFLAGS"])
    for offset, path in pairs[:-1]:
        indices = [i for i, value in enumerate(flags) if env.subst(str(value)) == str(path)]
        if len(indices) != 1 or indices[0] == 0 or int(flags[indices[0] - 1], 0) != offset:
            raise RuntimeError("Uploader and recorded flash plan disagree")
    manifest = dict(status="offline-validated; NOT hardware-approved or written",
                    build=json.loads((build / "generated/build-info.json").read_text()),
                    elf_sha256=elf_sha.hex(), app_limit=app[0].size, artifacts=records)
    (build / "artifact-plan.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("Keyboard artifact plan: ranges/uploader/ELF verified (no upload)")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", record_plan)
