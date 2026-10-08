# Licencscope és külső összetevők

A [LICENSE](LICENSE) MIT-engedélye a saját projektkódra és dokumentációra vonatkozik.
A `Copyright (c) 2023 bsvdoom` értesítés megmaradt; az új saját hozzájárulások
`Copyright (c) 2026 esp32-ble-laptop-keyboard contributors` alatt szerepelnek.
A saját adapter/state/session/ADC/diagnosztika, ellenőrzések és buildhookok ide
értendők. Az alábbi külső részeket és tisztázatlan történeti fájlokat ez nem
licenceli át. [MIT szöveg és azonosító](https://opensource.org/license/mit).

| Összetevő | Tényleges rögzített változat | Ellenőrzött licenc/eredet |
| --- | --- | --- |
| NimBLE-Arduino | 2.5.1, `5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb` | Apache-2.0; LICENSE és NOTICE az upstream Git-objektummal byteazonos |
| Vendorizált Apache Mynewt NimBLE | ugyanebben a commitban `src/nimble` | Apache-2.0, saját NOTICE, Apache/Espressif/Runtime attribúció |
| TinyCrypt | ugyanebben a commitban `src/nimble/ext/tinycrypt` | háromfeltételes BSD szöveg, Intel copyright; eredeti LICENSE megőrizve |
| CD74HC4067 | 1.0.2, gitlink `aa6959c25a9ad7160fd515f73d8727aa86fc1ffd` | Gyökér `LICENSE` MIT (`src/CD74HC4067.{h,cpp}:4` fejlécben LGPL-3.0 megjegyzéssel) |
| Arduino-ESP32 | 2.0.17; PIO `3.20017.241212+sha.dcc1105b` | vegyes: `cores/esp32/Arduino.h` LGPL-2.1-or-later, HAL-fájlok pl. ADC Apache-2.0, libb64 saját public-domain dedication |
| ESP-IDF / SDK | Arduino csomagba épített 4.4.7 | pl. ADC driver Apache-2.0, FreeRTOS V10.4.3 fejléc MIT; további SDK/runtime összetevők külön feltételekkel |
| Buildeszközök | Core6.2.0, espressif32 6.13.0, RISC-V8.4.0+2021r2-patch5, esptool4.11.0, SCons4.11.1 | külön upstream csomagok, nem a projekt MIT-licencének részei; ez nem teljes firmware/runtime licencjegyzék |

Elsődleges, verzióhoz kötött források:
[NimBLE LICENSE](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/LICENSE),
[NimBLE NOTICE](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/NOTICE),
[Mynewt NOTICE](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/src/nimble/NOTICE),
[TinyCrypt licenc](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/src/nimble/ext/tinycrypt/LICENSE),
[CD LICENSE](https://github.com/waspinator/CD74HC4067/blob/aa6959c25a9ad7160fd515f73d8727aa86fc1ffd/LICENSE),
[CD forrásfejléc](https://github.com/waspinator/CD74HC4067/blob/aa6959c25a9ad7160fd515f73d8727aa86fc1ffd/src/CD74HC4067.cpp),
[Arduino2.0.17](https://github.com/espressif/arduino-esp32/tree/2.0.17),
[IDF4.4.7](https://github.com/espressif/esp-idf/tree/v4.4.7).
Néhány teljes commit-URL webes nézete cache-miss hibás volt; a licencek
ellenőrzése a helyileg elérhető tényleges rögzített forrásokból is megtörtént.

## Követett NimBLE módosítás

`scripts/nimble-read-guard.patch` módosítja a pinned upstream
`src/NimBLELocalValueAttribute.h` és `src/NimBLEServer.cpp` fájlját: opcionális
read-guarddal érvénytelen battery read esetén ATT hibát adhat. Más attribútum
alapguardja nullptr. A patch **Apache-2.0** scope-ú, eredeti fejléceket megtart;
mindkét módosított fájlban projekt/módosítási értesítés van. A8/8 csak ezeket
az értesítéseket pontosította, a read-guard működését nem változtatta.

A patch eredeti és módosított bytehash-ei `scripts/nimble-read-guard.json`;
a forrás/revision/hash ellenőrzést és alkalmazást `scripts/prepare_build.py`
végzi. Az upstream licenc/NOTICE és vendorizált alkomponensek értesítései
byteazonos másolatként a `licenses/NimBLE-Arduino/` alatt is elérhetők.
Az upstream saját Git/LICENSE/NOTICE/header fájljait nem töröltük vagy írtuk át.
Apache-2.0 §4 eredeti licenc, módosítási értesítések és releváns NOTICE
megtartása; ezek a patch-elosztásban együtt kezelendők, nem csak a root MIT.

## Történeti scope, amelyre nem adunk új MIT-engedélyt

- `platformio_upload.py`: a régi, inaktív ElegantOTA segédscript eltávolításra
  került a munkafából (nem volt aktív buildhook vagy firmware-függőség).
- Régi HUN fork: nincs aktív forrás/gitlink/patch az új buildben. A magyar
  fizikai mátrix és a tesztorákulum numerikus HID usage adatok; az adapter
  önálló implementáció, nem a HUN ASCII decoder/descriptor másolata.
  A licenchiányos HUN-forrás csak privát `.local-snapshots` mentésben maradt,
  nem terjesztendő és nem kap új MIT-licencet. Régi történeti gitlink nem
  jelent engedélyt a külső fork átlicencelésére.

## Forrás és bináris külön

A saját MIT-forrás nem jelenti, hogy a teljes firmware.bin kizárólag MIT.
Bináris kiadás előtt az Arduino LGPL és egyéb belinkelt SDK/runtime-részek
forrás-/újralinkelési és értesítési kötelezettségeit külön rendezni kell.
Az itt megőrzött NimBLE-notice csomag sem a teljes SDK licencauditja.
Nincs automatikus bináris release vagy teljes bináris terjeszthetőségi
nyilatkozat. További részletek a [README](README.md#licencek) dokumentumban.
