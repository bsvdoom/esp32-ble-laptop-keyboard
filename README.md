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
build SUCCESS,523724 /26060 bájt. A billentyű/HID/session és battery native
trace-ek szintén PASS eredménnyel zárultak; lefedettségük lent szerepel. Ezek a kód és a fordíthatóság ellenőrzései,
nem igazolt fizikai működés vagy hostoldali vétel. A végső offline lezárás
(2026-10-08-i korábbi lezárás) csak dokumentációs egyezésellenőrzés volt.
Az ezt követő takarítás szükséges új normál buildjének eredménye lent szerepel;
változatlan runtime-teszteket nem ismételtünk.

| Ellenőrzés | Állapot |
| --- | --- |
| Windows / Linux / Android BLE-hostműködés | **NOT TESTED** mindhárom külön |
| Pairing, reconnect, host alvás/ébredés, hostoldali HID-vétel | **NOT TESTED** |
| Battery GATT / operációs rendszer százalék-kijelzése | **NOT TESTED** |
| ADC bekötés/pontosság/latency és százalékbecslés | **NOT TESTED** |
| Debounce hangolás, rövid nyomások, ghosting | **NOT TESTED** |
| Táp/RF, hatótáv, hosszú stabilitás | **NOT TESTED** |
| Távoli GitHub Actions futás | **NOT RUN**; CI statikusan ellenőrzött |

A saját MIT-implementáció kísérleti, hardveren nem validált forrásként
megosztható a dokumentált korlátokkal. A külső összetevők (SDK, NimBLE,
CD74HC4067) saját licencfeltételei a [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.md)
alatt szerepelnek.

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
GPIO2/8/9 strapping pinek: a muxterhelést reset/ROM-boot alatt is ellenőrizni
kell. GPIO18/19 USB-JTAG funkciója ütközhet a közvetlen oszlopokkal; a natív
USB diagnosztikát ne feltételezd működőnek ezen a pinouton. Ez hardverkockázat,
nem bizonyított magyarázat a régi fagyásra.
[IDF4.4.7 GPIO-korlátok](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32c3/api-reference/peripherals/gpio.html).

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
A központi `include/KeyboardConfig.h` paraméterei:20ms debounce,8 reportos
FIFO,20ms minimum küldési időköz,20…200ms exponenciális retry. Nincs
próbálkozásszám-limit, sikertelen release továbbra függő marad. Readiness
elvesztése/visszatérése és GAP-váltás új generációt ad, azonos handle újrafelhasználása
is megfigyelhető. Overflowkor release-barrier és
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

További pontos pinek a `platformio.ini`-ben: RISC-V8.4.0+2021r2-patch5,
esptool package2.41100.0 (API4.11.0), SCons4.41101.0 (4.11.1).
A Python tranzitív csomagokhoz nincs teljes hermetikus lock; a reprodukálhatóság
ugyanebből a deklarált forrásból kézi pótlás nélkül sikeres buildet jelent,
nem bitazonos image-et (UTC, Git-státusz és fordítási útvonalak eltérhetnek).
Az aktív NimBLE konfiguráció egyszerre egy kapcsolatot enged. A CI recursive
checkouttal és pinned Core-ral épít; az Actions v3/v4 tagek nem immutable commit-SHA-k,
ez a későbbi publikálás külön mérlegelendő CI-rögzítési kérdése, nem igazolt futáseredmény.
A build a követett `scripts/nimble-read-guard.patch` Apache-2.0 patchet
ellenőrzött eredeti/módosított hash alapján alkalmazza; eltérő kézi cache-állapotnál
megáll. A patch/meta és `prepare_build.py` együtt szükséges. Licencek/NOTICE-k
megőrizve a `licenses/NimBLE-Arduino/` alatt. A hook az exact NimBLE commitot
és mindkét érintett fájl hash-ét ellenőrzi; pristine állapotra git apply --check/apply,
ismert afterhash-re idempotens, kevert/eltérő cache esetén megáll és megőrzi azt.
A patch SHA256: `1d2208a1a01222eab0a0252afd86ccac409f0f14c4893a6d07a84b05f67970ed`.
Ez módosított upstream, nem tiszta
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
A0/100% clamping csak érvényes, konfigurált plausibility tartományú mérésre
vonatkozik; tartományon kívüli feszültség mérési hiba, nem automatikus végpont.
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
10s heartbeat kapcsolat nélkül is, uptime, heap/minheap, scan/loop idő/max,
connect/disconnect ok, authentication/encryption/subscription és korlátos
session/retry/overflow számlálók. A késői TX-státusz aggregált stackdiagnosztika,
nem az aktuális report/generáció ACK-ja. Nincs új watchdog/brownout/reset/TX-power
policy vagy saját BLE-stack-helyreállítás. Panic/watchdog/brownout megkülönböztetése
boot resetokkal és a teljes soros loggal vizsgálható; loghiány vagy tápvesztés
esetén ez sem teljes hibaforrás-bizonyíték. Nincs gépelt billentyűnapló. Engedélyezett ADC egyszeri indulási
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
visszaállítási terv alább található. Nem történt portnyitás vagy feltöltés.

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

## Célzott karbantartási ellenőrzések

A korábbi native trace-ek PASS eredményei: LED üres/egybyte-os adat; mind144
mátrixcella és magyar HID-usage egyezése; debounce press/release/pattogás és
időátfordulás; nyolc modifier, hat slot/hetedik visszafogadása, null/duplikált
usage; offline állapot, tartott gombok elnyomása, readiness és sessionváltás,
kezdeti üres report, elutasítás/retry/overflow/régi generáció; battery0/42/100,
invalid/stale/subscription/reconnect/retry; ADC osztó/modell/monotonitás/hibás
minták/filter/timestamp. Ezek a firmware tényleges feldolgozó kódját használó
szintetikus trace-ek, nem rádiós/GATT-host vagy hardveremuláció.

Egyszerű g++/C++11 ellenőrzések, új tesztkeret nélkül. Közös fordítási minta:

```sh
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iinclude tools/check_adc_battery.cpp src/BatteryAdcMath.cpp -o /tmp/keyboard-check-adc
ASAN_OPTIONS=detect_leaks=0 /tmp/keyboard-check-adc
```

| Ellenőrző fájl | Hozzáfordítandó firmwareforrás |
| --- | --- |
| tools/check_led.cpp | nincs, header-only helper |
| tools/check_hid.cpp | src/KeyboardState.cpp; tools/step2_hid_expected.h szükséges oracle |
| tools/check_state.cpp | src/KeyboardState.cpp |
| tools/check_session.cpp | src/KeyboardSession.cpp és src/KeyboardState.cpp |
| tools/check_battery.cpp | src/BatteryState.cpp |
| tools/check_adc_battery.cpp | src/BatteryAdcMath.cpp |

Az ASan/UBSan aktív volt; LeakSanitizer a környezet ptrace-korlátja miatt
kikapcsolt. Ne futtasd újra a teljes sort pusztán dokumentációs változás miatt.
A battery synthetic fixture és az ADC TEST CONFIGURATION hasznos, explicit
fordítási ellenőrzésekhez megmaradnak. Csak izolált projektmásolat build_flags
listájába kerüljön `-DKEYBOARD_BATTERY_COMPILE_CHECK` **vagy**
`-DKEYBOARD_ADC_TEST_CONFIGURATION`; a normál firmware egyiket sem használja,
a kettő keverése tiltott. A build-ID/JSON és az aktív header fingerprintje
külön jelöli ezeket; a tesztparaméterek nem helyettesítik a valós hardveradatokat.

## Feltételes első írás és visszaállítás

A `custom.csv` buildben feloldott tartományai (vég kizáró):

| Terület | Tartomány |
| --- | --- |
| Partíciós tábla szektora |[0x8000,0x9000) |
| NVS |[0x9000,0xD000) |
| otadata |[0xD000,0xF000) |
| phy_init |[0xF000,0x10000) |
| app0 |[0x10000,0x10AC80),1027200byte |
| app1 |[0x110000,0x1E0000) |

A framework8192byte-os boot_app0 két teljes otadata szektort inicializál,
seq1/0, UNDEFINED állapotokkal. A pinned IDF szerint seq1 az app0-t választja;
hibás appnál a bootloader más slotot is próbálhat. Ez offline elemzés, nem
igazolt boot. E000 offsettel a második szektor phy_init-et érintene; a posthook
D000-ra korrigálja **mind** FLASH_EXTRA_IMAGES, **mind** UPLOADERFLAGS értékét.
Nem változtatja a framework bytejait vagy a partíciókat.
[Arduino2.0.17 builder](https://github.com/espressif/arduino-esp32/blob/2.0.17/tools/platformio-build.py#L209),
[IDF4.4.7 bootválasztás](https://github.com/espressif/esp-idf/blob/v4.4.7/components/bootloader_support/src/bootloader_utility.c#L304).

Csak igazolt régi layout/mentés/kapacitás és adatmegőrzési döntés után lehet
első írást tervezni: négy külön image, a saját artifact-plan.json méret/SHA,
írási és4096byte-os törlési szektorhatáraival. Nincs teljes flashre padolt merged
image, gap-fill vagy automatikus erase-all. A bootloader/table/otadata/app0
érintett szektorai módosulnának; az új layout NVS/PHY/app1 területe csak azonos,
kompatibilis régi layout esetén őriz régi adatot. Ismeretlen/eltérő layout vagy
schema esetén nincs automatikus partíciócsere vagy NVS-migráció: előbb külön review.

**Első boot/pairing külön döntési pont:** Arduino2.0.17 initArduino
NO_FREE_PAGES/NEW_VERSION_FOUND esetén NVS-t formázhat; NimBLE2.5.1
REPEAT_PAIRING meglévő peer bondot törölhet. A projekt nem ad ehhez saját új
policyt, és nem garantálja a régi1.x bond-formátum kompatibilitását. A flashírás
NVS-t nem érintő terve önmagában nem garantál runtime-megőrzést. Ha minden
ilyen automatikus törlés kizárása követelmény, a jelen baseline nem teljesíti:
first boot előtt külön célzott döntés/javítás kell. A host meglévő párosításával
kezdd a későbbi próbát; sikertelenségkor naplóreview, nem automatikus forget/re-pair.
[Arduino NVS-init](https://github.com/espressif/arduino-esp32/blob/2.0.17/cores/esp32/esp32-hal-misc.c#L249),
[NimBLE repeat-pairing](https://github.com/h2zero/NimBLE-Arduino/blob/2.5.1/src/NimBLEServer.cpp#L608).

**Visszaállítás alapja:** ugyanazon modulon a hiteles teljes régi dump0x0-tól
a mért kapacitásig, később külön engedélyezett írás/readback/hash/boot művelettel.
Ez bootloadert/table/otadata/app/NVS-t együtt állítana vissza; eFuse-t és host
párosítási bejegyzéseket nem. Védelem/titkosítás/eFuse-állapot vagy hardverhiba
miatt a mentés megléte sem teljes visszaállítási garancia. Most nincs írási script.

A telepített **esptool4.11.0** read-only mintái; csak később ellenőrzött
adapter/port/táp és kézzel elért ROM-download mód után, egyszeri eljárásként.
A helykitöltőket nem szabad találgatott adatokkal kitölteni. A before/after
no_reset nem léptet bootmódba; no-stub ROM-olvasás, ALL a felismert teljes
kapacitás, nem a build2MB beállítása. Védett/olvashatatlan/eltérő dump vagy
bizonytalan kapacitás esetén állj meg, nincs force/védelemkiiktatás/eFuse-művelet.

```sh
KBD_PYTHON=/ellenorzott/build-venv/bin/python
KBD_ESPTOOL=/ellenorzott/pio-core/packages/tool-esptoolpy/esptool.py
KBD_PORT=ELLENORZOTT_PORT
KBD_BACKUP_DIR=/privat/tartos/mentes
"$KBD_PYTHON" "$KBD_ESPTOOL" --chip esp32c3 --port "$KBD_PORT" --baud 115200 --before no_reset --after no_reset --no-stub flash_id
"$KBD_PYTHON" "$KBD_ESPTOOL" --chip esp32c3 --port "$KBD_PORT" --baud 115200 --before no_reset --after no_reset --no-stub read_flash 0x0 ALL "$KBD_BACKUP_DIR/full-flash-1.bin"
"$KBD_PYTHON" "$KBD_ESPTOOL" --chip esp32c3 --port "$KBD_PORT" --baud 115200 --before no_reset --after no_reset --no-stub read_flash 0x0 ALL "$KBD_BACKUP_DIR/full-flash-2.bin"
sha256sum "$KBD_BACKUP_DIR/full-flash-1.bin" "$KBD_BACKUP_DIR/full-flash-2.bin"
cmp "$KBD_BACKUP_DIR/full-flash-1.bin" "$KBD_BACKUP_DIR/full-flash-2.bin"
```

A privát mentési mappa legyen új, Gitből kizárt,700 jogosultságú (dumpok600,
umask077). Mindkét méret a flash_id szerinti teljes kapacitással egyezzen,
két olvasás között ne bootoljon az app. Offline vizsgáld a régi image/table/
otadata érvényességét, slotválasztást és adatpartíció-metszeteket. A mai tábla
címe és NVS-séma nem bizonyítja a régiét; személyes dumpbyteokat ne publikálj.
[Esptool4.11 forrás/szintaxis](https://github.com/espressif/esptool/tree/v4.11.0).

## Hiteles buildmentések és folytatás

A Gitből kizárt `.local-snapshots/step-8-build-20261007/` a korábbi végső
normál és külön `adc-test/` teljes forrás–bin–ELF–build-info–artifact-plan–log
párját és SHA256SUMS/forrásmanifestjeit őrzi. Ez rekonstruált új firmware,
**nem** a kizárólag hardveren meglévő régi firmware dumpja vagy annak forrása.
A régebbi snapshotok is változatlanok. Az archivált utak későbbi cache nélküli
ellenőrzéséhez a helyi extraimage másolatokat és hash-eket használd, ne egy
újonnan fordított ELF-et vagy eltérő snapshot firmware-jét.

| 2026-10-07-i pár | Build-ID | Bin/ELF SHA256 |
| --- | --- | --- |
|normal|`no-git-unknown-src7a32e9a39cd5`|bin `9d4dfadee8cfbab7888a7eec814cd0a91510ce41f50ec2a651d7279f788e41b1`; ELF `27a27869d64d1e259a4dc63580384d78cc721ad6f91b58ea4d6e24b0983bdefd`|
|ADC TEST CONFIGURATION|`no-git-unknown-src2d06451e9e9d-ADC-TEST-CONFIGURATION`|bin `fe5c7b1aee41ea5541cb3b1bfed695d7737d735a7008db4a4e8cb5cc6a5807c7`; ELF `4855b6ea7f2b3bffa36ad26af3c4c9d549131f6c2212cb467f4e1d1b2095ecb4`|

A takarítás az inaktív cert és üres include-sablon eltávolításával a tartalmi
buildazonosítót is érinti. Az ehhez szükséges új normál build eredményét és
külön mentését az alábbi lezáró sor rögzíti; a fenti régi párok nem íródnak át.

**Takarítás utáni normál build —2026-10-08:** SUCCESS, 518328byte flash /
25772byte statikusRAM (a korábbi normálhoz +8 /+0),15,57s;
friss buildkönyvtár, meglévő pontos csomagcache, nincs letöltés/frissítés.
Hat korábbi upstream warning, saját forráshiba nincs; runtime-teszt nem ismétlődött.
Build-ID `fc0f5868ec8c-dirty-src752760c2affb`, UTC `2026-10-08T01:11:49Z`;
bin 544656byte, SHA256 `21949f398e00882104fa78d4c40151a30cfd0d5290b77f2224efc44118862eec`;
ELF SHA256 `144ab8aa997fed525611a7ed5372ec24bc27896de82514cd4ee4e12a4c4d4ef0`. A dirty jelölés a valós munkafaállapot.

Új hiteles normál forrás–bin–ELF–extraimage–build-info–artifact-plan–log mentés:
`tmp/publication-cleanup-20261008/final-build/`, forrásmanifest és SHA256SUMS
mellett, Gitből kizárva. A régi `.local-snapshots` és ADC TEST CONFIGURATION
pár változatlan; azokkal ezt ne keverd. A README a friss maintenanceszöveg,
a forrásarchive ugyanahhoz az aktuális runtime-fingerprinthez tartozik.

Az új normál build tényleges image/törlési terve (minden vég kizáró):

| Artifact | Offset / byte | Írási tartomány | Érintett törlési szektorok |
| --- | --- | --- | --- |
|bootloader.bin|0x0 /13248|[0x0,0x33C0)|[0x0,0x4000)|
|partitions.bin|0x8000 /3072|[0x8000,0x8C00)|[0x8000,0x9000)|
|boot_app0.bin|0xD000 /8192|[0xD000,0xF000)|[0xD000,0xF000)|
|firmware.bin|0x10000 /544656|[0x10000,0x94F90)|[0x10000,0x95000)|

A konkrét image-SHA-k az adott mentés artifact-plan.json-jában vannak; a binbe
ágyazott ELF SHA és valamennyi határ/hash/uploader-pár offline ellenőrzött.
A három extraimage byteazonos a korábbi step-8 mentéssel. Ez nem hardveres
írási vagy régi-layout migrációs jóváhagyás.


A munkát önállóan innen lehet folytatni: aktív config/src/include/scripts/tools
és licencek a projektben; hardveres sorrend fent, első írás/readback/restore
feltételek ebben a README-ben. A történeti auditnaplók nem szükségesek a buildhez
vagy a folytatáshoz. A takarítás helyi ellenőrzőpontja és információtérképe:
`tmp/publication-cleanup-20261008/checkpoint.json` (Gitből kizárt, nem publikálandó).

## Licencek

A saját projektkód [MIT](LICENSE), megőrzött2023 copyrighttal és2026contributors
értesítéssel. A külső SDK/CD/NimBLE és az Apache-2.0 patch **nem MIT-re
átlicencelt**; a konkrét upstream licencek és NOTICE-k a
[THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.md) és `licenses/` alatt maradnak.
A régi, inaktív `platformio_upload.py` OTA-segéd eltávolításra került.

Új, tiszta repóba publikáláskor minden aktív forrás/config/hook/patch+JSON,
célzott ellenőrzés, pontos CD gitlink/.gitmodules, requirements és licencek
átadandók; a privát snapshot/cache/dump/bin/ELF/log/secret és eFuse/hostadat
nem publikálandó.

Örökölt PnP VID/PID és gyártóstring van (`src/KeyboardHid.cpp`), saját vendor
hozzárendelés nincs igazolva: kereskedelmi/végleges eszközazonosítás előtt
külön rendezendő. BAS nélküli HID-profil HOGP-minősítése nem igazolt.
Firmware.bin terjesztése előtt az Arduino LGPL és belinkelt SDK/runtime
forrás-/újralinkelési/értesítési feltételei külön review-ra maradnak;
a saját MIT és az őrzött NimBLE-notice csomag nem teljes bináris licencaudit.
Nincs automatikus bináris release, portművelet, feltöltés vagy publikálás.
