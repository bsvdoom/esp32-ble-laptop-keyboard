# ESP32-C3 BLE laptopbillentyűzet

Régi laptop 8×18 mátrixa két CD74HC4067 multiplexerrel, ESP-C3-12F modulon.
Rekonstruált, OTA nélküli projektbeli HID-adapter Arduino2.0.17 / IDF4.4.7 és
NimBLE-Arduino2.5.1 fölött. Nem az eredeti hardverfirmware reprodukciója.
Nincs Wi-Fi, `secret.h`, webes függőség vagy karbantartási mód.

**Offline build és célzott native trace-ek ellenőrizve.** A hardveres
validáció elhalasztott. A korábbi fagyás oka és megszűnése nem bizonyított;
javított hatótáv sem igazolt. A Windows/Linux/Android kompatibilitás cél,
nem ellenőrzött eredmény.

A 2026-10-07-i végső ellenőrzések eredménye: a célzott ADC-modell native
ellenőrzése PASS (ASan/UBSan); a tiszta, izolált normál C3 build SUCCESS,
518320 bájt flash /25772 bájt statikus RAM; a külön ADC TEST CONFIGURATION
build SUCCESS,523724 /26060 bájt. A korábbi billentyű/HID/session trace-ek
eredményei a PROGRESS-ben vannak. Ezek a kód és a fordíthatóság ellenőrzései,
nem igazolt fizikai működés vagy hostoldali vétel. A végső offline lezárás
(2026-10-08) csak dokumentációs egyezésellenőrzés; új build vagy runtime-teszt nem futott.

| Ellenőrzés | Állapot |
| --- | --- |
| Windows / Linux / Android BLE-hostműködés | **NOT TESTED** mindhárom külön |
| Pairing, reconnect, host alvás/ébredés, hostoldali HID-vétel | **NOT TESTED** |
| Battery GATT / operációs rendszer százalék-kijelzése | **NOT TESTED** |
| ADC bekötés/pontosság/latency és százalékbecslés | **NOT TESTED** |
| Debounce hangolás, rövid nyomások, ghosting | **NOT TESTED** |
| Táp/RF, hatótáv, hosszú stabilitás | **NOT TESTED** |
| Távoli GitHub Actions / Docker futás | **NOT RUN**; CI statikusan ellenőrzött |

A teljes repo még nem feltétel nélkül publikálásra kész: konkrét külső
licenc-/eredetkérdések maradtak. **A felhasználó külön kezeli** a CD74HC4067
licenceltérését és az új, tiszta repóba publikálást, beleértve a régi OTA-segéd
sorsát. Ezek továbbra nyitott feladatok, nem megoldott eredmények; itt nem
készült új repó és nem változott Git-előzmény. A saját MIT-forrás megoszthatóságát és a
fennmaradó scope-ot a [publikálási audit](PUBLICATION_AUDIT.md) külön mérlegeli.

## Hardver és pinout

A pinértékek változatlanok, közös helyük `include/KeyboardPins.h`.
A kiosztás kódból ellenőrzött terv, nem igazolt fizikai bekötés.

| Funkció | GPIO / csatorna |
| --- | --- |
| Sormux jel |6, OUTPUT HIGH |
| Sormux S0/S1/S2 |9/8/5; S3-at a program nem hajtja |
| Oszlopmux jel |7, INPUT_PULLDOWN |
| Oszlopmux S0/S1/S2/S3 |4/3/2/1 |
| Mux sorok |0–7 |
| Mux oszlopok |0–15 |
| Két közvetlen oszlop |18/19, INPUT_PULLDOWN |
| Soros diagnosztika |UART TX21/RX20,115200 baud |
| Opcionális ADC1 |GPIO0 kódszinten szabad jelölt;1–4 foglalt; ADC2 tiltott |

Sormux S3 LOW és mindkét mux enable/E aktív LOW bekötése, 3,3V-os jelszintek,
közös GND és tápellenőrzés szükséges. Ezek tényleges meglétét a kód nem bizonyítja.
A20µs beállási idők változatlanok. Diódázás/ghostingmentesség nincs igazolva.

## Billentyűzet viselkedése

A hoston **magyar kiosztást** válassz: fizikai HID usage-okat küldünk,
nem Unicode-karaktereket. Mind a nyolc bal/jobb modifier külön bit,
hat egyedi normál usage a reportban. Több fizikai cella azonos usage-a egyszer
szerepel. A hatgombos választás megtartja a tartott korábbi slotokat,
felszabadult helyre sor–oszlop sorrendben felveszi a tovább tartott kimaradt gombot.
Nincs NKRO vagy új médiafunkció.

Offline is folyamatos a scan és a kezdeti20ms, cellánkénti press/release debounce.
Bontás után, első connectkor vagy új ready munkamenetben már tartott gomb és
modifier **stabil felengedés + új lenyomás** után működik. Offline gépelést
nem játszunk vissza. Readinesshez kapcsolat, titkosítás és keyboard subscription
kell. Új munkamenet először üres reportot próbál, helyi elfogadás után folytatja.
Nyolcelemű FIFO és korlátos időzített retry van; overflowkor release-barrier és
legfrissebb állapot marad, köztes nyomások elveszhetnek. Megszakadt kapcsolatban
a régi hostnak release nem garantálható. Notify bool/TX-státusz helyi stackjelzés,
nem host ACK; a handle-only API check→notify versenyének korlátja megmarad.

## Build és függőségek

Git, Python3.9+ és az első csomagletöltéshez internet szükséges. Linux/macOS példa:

```sh
git submodule update --init --recursive
python3 -m venv /tmp/keyboard-build-venv
/tmp/keyboard-build-venv/bin/python -m pip install -r requirements-build.txt
/tmp/keyboard-build-venv/bin/pio run -e esp32c3
```

| Összetevő | Rögzítés |
| --- | --- |
| PlatformIO Core |6.2.0, requirements-build.txt |
| Platform |espressif32@6.13.0 |
| Framework |Arduino2.0.17 / IDF4.4.7,3.20017.241212+sha.dcc1105b |
| NimBLE |2.5.1, commit5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb |
| CD74HC4067 |gitlinkaa6959c25a9ad7160fd515f73d8727aa86fc1ffd, metadata1.0.2 |

Toolchain/esptool/SCons is pontosan rögzített a `platformio.ini`-ben.
A build a követett `scripts/nimble-read-guard.patch` Apache-2.0 patchet
ellenőrzött eredeti/módosított hash alapján alkalmazza; eltérő kézi cache-állapotnál
megáll. A patch/meta és `prepare_build.py` együtt szükséges. Licencek/NOTICE-k
megőrizve a `licenses/NimBLE-Arduino/` alatt. Ez módosított upstream, nem tiszta
NimBLE byteállapot. [PIO dependency pinning](https://docs.platformio.org/en/latest/librarymanager/dependencies.html).

Az esp32-c3-devkitm-1 **fordítási preset**, nem az ESP-C3-12F fizikai azonossága.
CPU160MHz, flash-beállítás2MB/QIO/80MHz és custom.csv örökölt, hardveren nem
ellenőrzött; az image DIO fejléce és preset külön adat. A valódi kapacitást
nem bizonyítja a build. App0 limit1027200 byte. A CI csak recursive checkout,
pinned Core és C3 build; nem futtat hardvertesztet vagy bináris release-t.

## Opcionális LiPo1S2P akkumulátor

A felhasználói modell: **1 soros,2 párhuzamos cella**,3200mV=0%,3700mV=50%,
4200mV=100%. A párhuzamos cellák nem kétszerezik a feszültséget.
A 3200–4200 mV tartományban a becslés lineáris, egész százalékra kerekítve.
A „Rmin/Rmax” itt feszültség-végpont, nem ellenállás. Ez kerekített lineáris
**feszültségbecslés**, nem kapacitásmérés, töltésvezérlés vagy BMS.
A3200mV-os pont nem hardveres lekapcsolási policy.

```text
akku+ ── R_top ── ADC csomópont ── R_bottom ── közös GND
                       │
               ellenőrzött ADC1 GPIO
```

**Alapból kikapcsolt**: `include/BatteryAdcConfig.h`, `enabled=false`.
A `cells=1` a soros cellaszám; párhuzamos mező nem szükséges a feszültségmodellhez.
Az Rt/Rb/tolerancia, tényleges ADC-bekötés, maximális fizikai packfeszültség és
plausibility alsó határ továbbra ismeretlen. A modell4200-as végpontja nem
helyettesíti a töltő toleranciával növelt fizikai maximumát. A hiányzó
mezők0/−1/UNKNOWN értékkel megakadályozzák a mérés engedélyezését.

Konfigurációs mezők: `gpio`, IDF `attenuation` index0/1/2/3; `rTopOhm` és
`rBottomOhm`; `cells`, `maxBatteryMv`; `emptyCellMv`, `fullCellMv`,
`minPlausibleCellMv`; `resistorTolerancePermille`, `headroomPercent`;
`periodMs`, `sampleSpacingMs`, `samples`, `maxBurstMs`, `rawRailMargin`,
`maxSpreadCellMv`; multiméteres `gainPpm`/`offsetMv` korrekció.
Csak megadott és review-zott hardverparaméterekkel állítsd enabled=true-ra.

`V_bat=V_adc×(Rt+Rb)/Rb`; osztóáram `I=V_bat/(Rt+Rb)`.
A legrosszabb toleranciás arány után legalább10% mérési tartalék kell.
C3 ADC-tartomány felső határa750/1050/1300/2500mV az attenuation indexek szerint;
az index3 telepített IDF neve ADC_ATTEN_DB_12, régi DB_11 alias.
Ez **mérési tartomány**, nem abszolút elektromos terhelhetőség.
[IDF4.4.7 ADC](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32c3/api-reference/peripherals/adc.html).

Az ADC1 driver legfeljebb egy konverziót végez looponként a keyboard után;
alap30s periódus,4 időben elosztott minta, legalább25ms térköz. EFuse TP
kalibrációt használ, nincs nyers ADC/4095 vagy default-Vref fallback.
Hiányzó kalibráció, telítettség, hibás/szórt/elavult minta érvénytelen adat,
nem automatikus0/100%. Lebegő bemenet biztos felismerése nincs.
[Arduino2.0.17 API-forrás](https://github.com/espressif/arduino-esp32/blob/2.0.17/cores/esp32/esp32-hal-adc.c).

A szabványos0x180F/0x2A19 READ/NOTIFY Battery Service csak bekötött backenddel
aktív; nélküle az automatikus NimBLE BAS advertising előtt kikerül.
Battery subscription/retry külön van a keyboard FIFO-tól. Változatlan értéket
nem notify-zunk folyamatosan; reconnect/resubscribe után a friss aktuális adat
közölhető. Hibás/hiányzó/stale read ATT0x0E hibát kap, nem százalék-sentinelt.
A GATT-közlés nem bizonyítja az OS Bluetooth felületének százalék-kijelzését.

**TEST CONFIGURATION**: `tools/adc_test_configuration.h`, csak a külön
`KEYBOARD_ADC_TEST_CONFIGURATION` makróval.100kΩ/100kΩ,1% tolerancia,
4250mV maximum és2800mV plausible minimum hipotetikus, nem végleges hardveradat;
a3200/4200-as modell viszont a felhasználói döntést követi. Példaosztóval
4250mV→2125mV, legrosszabb arány2146,25mV<2250mV tartalékos limit;
áram21,25µA. Az image ADC-TEST-CONFIGURATION jelölésű, külön megőrzött.

Bemeneti bypass kapacitás (például100nF ADC–GND) zajcsökkentési lehetőség;
a nagy ellenállások terhelése és RC beállás hardveren ellenőrzendő.
[Espressif zajcsökkentés](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32c3/api-reference/peripherals/adc.html#minimizing-noise).
Multiméterrel több ponton hasonlítsd össze az akku/ADC feszültségét a cache
mV-jával, majd szükség szerint állítsd gain/offsetet. Terhelés, töltés,
hőmérséklet és akkutípus eltérítheti a százalékbecslést.
[TI SOC-becslési korlátok](https://www.ti.com/lit/an/sluaaa1/sluaaa1.pdf#page=4).

## Diagnosztika és első feltöltés előtt

115200 baud, boot build-ID/dirty vagy no-git állapot+forrásfingerprint/resetok,
10s heartbeat/uptime, heap/minheap, scan/loop max-idő és korlátos BLE/session
megfigyelések. Nincs gépelt billentyűnapló. Engedélyezett ADC egyszeri indulási
status sort ad; a runtime cache/status/mV gettereken elérhető.
Később ellenőrzött porttal például:

```sh
/tmp/keyboard-build-venv/bin/python bin/bbread.py ELLENORZOTT_PORT /privat/naplomappa/serial.BFL
```

A bin–ELF–build-ID párt **ugyanahhoz a buildhez** őrizd meg a loggal és SHA256-tal;
hiba után ne egy új fordítás ELF-jét használd. A `.pio/build/esp32c3/generated/`
metadata és `artifact-plan.json` is mentendő. Privát mentések/dumpok/logok és
image-ek Gitből kizártak. A végső helyi mentés: `.local-snapshots/step-8-build-20261007/`.

**Ne indíts vak uploadot.** Az eredeti firmware csak a hardveren van.
Első írás előtt két egyező, teljes kapacitású privát flashmentés; tényleges
flashkapacitás/bootloader/partíciós layout/otadata és NVS/bond állapot vizsgálata
szükséges. A jelen upstream Arduino NVS-init és NimBLE repeat-pairing bizonyos
helyzetekben automatikusan törölhet adatot; megőrzésre első boot előtt külön
review kell. Nincs automatikus erase/bondtörlő/migrációs projektpolicy.

A posthook az initializer alap E000 offsetjét a custom.csv otadata-jához
**D000**-ra igazítja. Offline ellenőrzött terv: bootloader.bin@0000,
partitions.bin@8000, boot_app0.bin@D000, firmware.bin@10000. Megmaradtak az
OTA-slotok, OTA runtime nincs. A régi layoutot ez nem bizonyítja; csak az adott
build hash/határ/erase/ELF-egyezéses terve. A részletes feltételes első-írási és
visszaállítási terv a PROGRESS7/A-ban van. Nem történt portnyitás vagy feltöltés.

## Későbbi hardveres ellenőrzés sorrendje

Minden alábbi lépés elhalasztott, **NOT TESTED**; a lista nem feltöltési parancs.

1. **Régi teljes flash mentése első írás előtt:** ellenőrzött adapterrel,
   jelszintekkel és ROM-download móddal azonosított valódi kapacitásról két
   egyező olvasás, privát mentés, méret- és SHA256-ellenőrzés.
2. A mentésből a régi bootloader/layout/otadata/appslotok/NVS/bond tartományok
   vizsgálata; az első boot adatmegőrzési és visszaállítási tervének review-ja.
   A host párosítási adatbázisa és az eFuse nem része a flashdumpnak.
3. A tényleges pinout, mux enable/S3, táp/jelszintek és flashváltozat ellenőrzése.
   ADC-hez Rt/Rb/tolerancia, választott ADC1 GPIO, fizikai maximum és
   plausibility megadása; a TEST CONFIGURATION nem valós bekötés.
4. Külön későbbi feltöltési utasítás és ellenőrzött artifact/offset terv után
   indulás/115200 baud heartbeat/resetok/heap/időmérés; az adott bin–ELF–ID
   és napló megőrzése. Nincs automatikus erase-flash vagy bondtörlés.
5. Windows/Linux/Android hoston külön magyar gépelés, nyolc modifier,
   hatgombos működés, gyors gépelés, tartott gombos reconnect és alvás/ébredés.
   Csak a ténylegesen elérhető host eredménye rögzíthető ellenőrzöttként.
6. A valós paraméterekkel engedélyezett ADC multiméteres ellenőrzése/kalibrációja,
   Battery GATT és host UI külön ellenőrzése; debounce/ghosting, valamint
   megfigyelt tünet alapján táp/RF és tartós stabilitás vizsgálata.

## Ellenőrzések és licencek

A célzott matematikai/native trace-ek történeti eredményei a PROGRESS-ben,
a végső megváltozott ADC-modell ellenőrzése:

```sh
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iinclude tools/check_adc_battery.cpp src/BatteryAdcMath.cpp -o /tmp/keyboard-check-adc
ASAN_OPTIONS=detect_leaks=0 /tmp/keyboard-check-adc
```

A saját projektkód [MIT](LICENSE), megőrzött korábbi copyrighttal.
A külső SDK/CD/NimBLE és az Apache-2.0 patch **nem MIT-re átlicencelt**.
A CD root MIT vs LGPL-fejléc eltérés és az inaktív régi uploader eredete nyitott;
a rendezésüket és a tiszta repóba publikálást a felhasználó külön intézi.
Firmware.bin terjesztési megfelelőségét nem állítjuk teljesnek.
[THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.md), [PUBLICATION_AUDIT](PUBLICATION_AUDIT.md).
Részletes történeti bizonyítékok: [AUDIT](AUDIT.md), [PROGRESS](PROGRESS.md).
