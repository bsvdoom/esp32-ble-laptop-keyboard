> **Aktuális állapot (2026-10-08): végső offline lezárás kész. A CD-licenceltérést és az új, tiszta repóba publikálást, beleértve a régi OTA-segédet, a felhasználó külön kezeli; ezek nyitottak. Hardveres elfogadás elhalasztott. Feltöltés/commit/publikálás nincs.**

# Felújítás — 1/8: rekonstruált BLE-kiindulópont

Dátum: 2026-10-06. **Állapot: 1/8 kész; 2/8 nem kezdődött el.** Ez nem az eredeti firmware reprodukciója: az eredeti secret.h, image/ELF és bizonyított függőséglista nincs meg. Az AUDIT.md a változtatás előtti állapot dokumentuma; változatlanul megőrzöm.

## Elfogadott döntések és határok

- Hardver: ESP-C3-12F, meglévő laptopbillentyűzet és multiplexerek; pinout, 8×18 mátrix és magyar kiosztás marad.
- Célhostok: Windows, Linux, Android; most nincs igazolt hostkompatibilitás vagy BLE-stabilitás.
- OTA most elhagyható; BLE-buildhez nem kell secret.h, Wi-Fi-adat vagy webes könyvtár.
- Reconnect után a bontáson át tartott gomb csak felengedés és új lenyomás után működjön. **Most csak döntésként dokumentálva; nincs implementálva.**
- A végleges BLE-könyvtár választása 3/8-ban történik. Most a rögzített HUN fork és NimBLE1 kompatibilis baseline-ja a cél, nem migráció.
- Csak a célprojekt módosítható. Nincs unit teszt, referencia-build, feltöltés, flash/NVS-törlés, publikálás vagy push. A teszthardveren lévő régi firmware megmarad.

## Kiinduló ellenőrzés

Az alkalmazható AGENTS.md keresése a workspace és szülőútvonal alatt nem talált utasításfájlt. Kezdeti célprojekt `git status --short --untracked-files=all`: csak `?? AUDIT.md`; követett felhasználói módosítás nincs. A három referencia munkafája tiszta. A meglévő auditot nem változtatom.

## Változtatások és ellenőrzések

- A CD74HC4067 és HUN könyvtárak meglévő gitlinkje és upstreamje maradt; a HUN útvonal kis/nagybetű hibája javítva. Mindkét submodule inicializálva, forrásmódosítás nélkül.
- A nem létező/használatlan ArduinoJson .gitmodules-bejegyzés és az ElegantOTA bejegyzés/gitlink eltávolítva. Az OTA gitlink törlése a Git indexben van, a többi változás egyelőre nincs stage-elve; commit nem készült.
- A src/main.cpp fájlból csak a secret.h include és az OTA/Wi-Fi ág, callbackek és bekötésük kerültek ki. A BLE kapcsoló most követett buildflag, nem hiányzó secretből származik.
- NimBLE-Arduino 1.4.3 és a buildcsomagok pontos verziója rögzítve, az espressif32 6.0.1 platform megmaradt. A nem használt native/ArduinoFake környezet kikerült; unit teszt nem készült/futott.
- requirements-build.txt rögzíti a Core 6.2.0-t; a Docker ezt telepíti (gyökér buildkontextussal). A hibás régi CI native-teszt/többplatformos/4 MB-os merge ág helyett csak rekurzív submodule checkout és C3 build maradt. Docker/remote CI most nem tesztelt.
- README.md tartalmazza az önálló build menetét és korlátait.
- A Core 6.2.0 telepítése izolált /tmp venvbe sikerült. A korlátozott hálózat DNS-hibája után a pip és git letöltések hálózati engedéllyel sikerültek. A sandboxos PIO-csomagletöltést 130-as kóddal megszakítottuk; a tényleges baseline-build hálózati engedéllyel sikerült (exit 0, 81,85 s).

### Első sikeres baseline-build

Parancs a célprojekt gyökerében:

```sh
PLATFORMIO_CORE_DIR=/tmp/keyboard-step1-20261006/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step1-20261006/build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step1-20261006/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
```

Eredmény: **SUCCESS, exit 0**; flash 508300 / 1027200 byte (49,5%), statikus RAM 25548 / 327680 byte (7,8%). A 1027200 byte maximum a változatlan app0 partícióból számolt buildkorlát, nem a modul fizikai flashméretének mérése. A platform, framework, toolchain, esptool, SCons és NimBLE frissen letöltve; a régi .pio nem használt. A két helyi submodule tiszta, a fork kódja változatlan. Nincs compiler warning/error a sikeres build naplójában. A GPIO/mux globális deklarációk, a teljes keyMatrix és readCols/writeRows HEAD-hez képest szövegesen egyeznek; custom.csv változatlan. Ez diff-ellenőrzés, nem unit teszt.


### Pontos deklarált és feloldott függőségek

A `pio pkg list -e esp32c3` a fent megadott izolált Core/build/libdeps változókkal exit 0-t adott. A build függőségi gráfja pontosan három könyvtárat használ; a helyi git submodule-ok nem szerepelnek a Registry-könyvtárakat felsoroló `pkg list` Libraries ágában, ezért a gitlink és a buildgráf együtt ellenőrizve.

| Összetevő | Rögzített forrás / verzió | Tényleges eredmény |
| --- | --- | --- |
| PlatformIO Core | PyPI, requirements-build.txt: 6.2.0 | 6.2.0, izolált Python 3.14.4 venv |
| Platform | PlatformIO Registry, espressif32@6.0.1 | 6.0.1, változatlan platform |
| Arduino framework | platformio/framework-arduinoespressif32@3.20006.221224 | Arduino-ESP32 2.0.6, beépített ESP-IDF 4.4.3 |
| RISC-V toolchain | espressif/toolchain-riscv32-esp@8.4.0+2021r2-patch5 | GCC 8.4.0, ugyanaz a csomag |
| esptool | platformio/tool-esptoolpy@1.40400.0 | 4.4.0 |
| SCons | platformio/tool-scons@4.41101.0 | 4.11.1 |
| NimBLE-Arduino | Registry h2zero/NimBLE-Arduino@1.4.3 | 1.4.3, nincs git checkout; upstream release commit e2b40749ef1e1aae9b1f5b2647d2bbadc5abb44f |
| CD74HC4067 | git submodule, waspinator/CD74HC4067 | aa6959c25a9ad7160fd515f73d8727aa86fc1ffd, metadata/tag 1.0.2 |
| HUN BLE Keyboard fork | git submodule, bsvdoom/ESP32-BLE-Keyboard-HUN | 5886c43812b81303501017aa39f91d6e775bf93e, library.properties 0.3.3; headerben eltérő régi 0.0.5 jelölés |

A Core Python-függőségeinek tényleges listája ideiglenesen `/tmp/keyboard-step1-20261006/python-packages.txt` alatt van. A követett deklaráció a Core-t és a firmware-build minden használt külső csomagját/könyvtárát rögzíti; a Python tranzitív csomagok, operációs rendszer és CI actionök nem teljesen hermetikus lock részei. A két forrásból történő sikeres fordítás ellenőrzése a cél, nem bitazonos binárisra tett ígéret.

Az esptool régi Python-forrása Python 3.14 alatt `SyntaxWarning: 'return' in a 'finally' block` figyelmeztetést adott; ez nem C/C++ compiler warning és nem akadályozta a buildet. A forkhoz, mátrixolvasáshoz és Arduino/NimBLE API-hoz nem kellett kompatibilitási forráspatch.

### Külső döntések elsődleges forrásai

- A pontos, ownerrel egyértelműsített Registry pin és a helyi könyvtárforrás a [PlatformIO függőségkezelés](https://docs.platformio.org/en/latest/librarymanager/dependencies.html) által támogatott forma; a framework/tool csomagok rögzítése a [platform_packages](https://docs.platformio.org/en/latest/projectconf/sections/env/options/platform/platform_packages.html) felülírásaival történik. A már létező gitlinkek commitpint adnak; nem vezetünk be második Registry/Git forrást ugyanahhoz a könyvtárhoz.
- Az [espressif32 6.0.1 kiadás](https://github.com/platformio/platform-espressif32/releases/tag/v6.0.1) Arduino 2.0.6-ot támogat. Ezt megtartottuk; az ESP-IDF 4.4.3 a letöltött Arduino SDK fejlécében ellenőrzött verzió, nem külön ESP-IDF framework-migráció.
- A [NimBLE-Arduino 1.4.3 kiadás](https://github.com/h2zero/NimBLE-Arduino/releases/tag/1.4.3) és [release commit](https://github.com/h2zero/NimBLE-Arduino/commit/e2b40749ef1e1aae9b1f5b2647d2bbadc5abb44f) az audit NimBLE1-jelöltjét azonosítja. A HUN fork API-kompatibilitásának helyi bizonyítéka a sikeres fordítás; futási stabilitást ez nem igazol. NimBLE2/Arduino3 migráció vagy végleges BLE-választás nem történt.
- [CD74HC4067 upstream a megtartott commiton](https://github.com/waspinator/CD74HC4067/tree/aa6959c25a9ad7160fd515f73d8727aa86fc1ffd), [HUN fork a megtartott commiton](https://github.com/bsvdoom/ESP32-BLE-Keyboard-HUN/tree/5886c43812b81303501017aa39f91d6e775bf93e). A webes nézetek cache-hibája mellett a deklarált upstreamből történő Git-letöltés és a commit checkout sikeres volt.

### ESP-C3-12F és a board preset ellenőrzése

Az Ai-Thinker [ESP-C3-12F V1.1 angol adatlapjának](https://docs.ai-thinker.com/_media/esp32/docs/esp-c3-12f_specification.pdf) keresőben indexelt gyártói részlete ESP32-C3/RISC-V, legfeljebb 160 MHz CPU-t ír le. A [kínai V1.1 gyártói adatlap](https://docs.ai-thinker.com/_media/esp32/docs/esp-c3-12f%E8%A7%84%E6%A0%BC%E4%B9%A6.pdf) indexelt paramétertáblája 4 MB alapkiépítést és 2 MB változatot, továbbá GPIO0–10/18/19/20/21 portokat sorol. **Korlát:** az angol PDF-link teljes megnyitása jelenleg általános HTML dokumentációs kezdőlapot ad; nem állítjuk, hogy a teljes adatlap vagy a konkrét modulváltozat ellenőrzött. Az indexelt kivonat a C3-preset megtartását támogatja, de önmagában nem igazolja a teszteszköz flashméretét vagy tényleges bekötését.

Az [Espressif DevKitM-1 hivatalos leírása](https://docs.espressif.com/projects/esp-idf/en/v4.4.3/esp32c3/hw-reference/esp32c3/user-guide-devkitm-1.html) más fizikai hardvert ír le: ESP32-C3-MINI-1, ESP32-C3FN4, 4 MB flash és saját USB–UART/RGB LED áramkör. A letöltött [PlatformIO 6.0.1 board-definíció](https://github.com/platformio/platform-espressif32/blob/v6.0.1/boards/esp32-c3-devkitm-1.json) helyben ellenőrizve esp32c3 MCU/variant, 160 MHz CPU, QIO/80 MHz flash, alapból 4 MB; a projekt ezt továbbra is 2 MB-ra írja felül. A build által kiírt `2MB Flash` konfigurált érték, nem autodetekció.

Következtetés: a meglévő presetet C3-fordítási alapként megtartjuk, nem tekintjük az ESP-C3-12F hardverének bizonyított modelljének. Nem változott a GPIO, `custom.csv`, 2 MB, QIO/80 MHz, `/dev/ttyUSB1`, 921600 feltöltési vagy 115200 monitor-sebesség. A fizikai flashváltozat, flash üzemmód/órajel alkalmassága, adapter/port és boot-bekötés nincs igazolva. A GPIO2/8/9 strapping és GPIO18/19 USB funkciókat az Espressif dokumentum megerősíti; a projekt mátrixához kapcsolt szerepük változatlan, későbbi hardveres ellenőrzést igényel. A CPU/MCU szinten megfelelő fordításból nem következik, hogy az image már feltölthető a konkrét teszteszközre.

### Izolált, tiszta másolat buildje

A másolat nem a teljes munkakönyvtár fájlmásolása: helyi `git clone --no-hardlinks` az eredeti HEAD-ről, majd `git diff HEAD --binary` patch alkalmazása `git apply --index` paranccsal. Kizárólag az explicit új `requirements-build.txt` és az ellenőrzőpontot tartalmazó `PROGRESS.md` került még bele, a másolat indexében követettként. Nem másoltuk az untracked auditot, régi .pio-t, titkot vagy könyvtárat. A build input tree 90f4f8920ffed439f5ac8554bda1962177ff8f84; a végső PROGRESS.md később az eredményekkel bővült, a build forrása és konfigurációja azóta nem változott.

Lényeges futtatott parancsok:

```sh
# A célprojekt gyökerében:
git diff HEAD --binary > /tmp/keyboard-step1-20261006/candidate.patch
git clone --no-hardlinks /home/adam/Projects/git/keyboard/esp32-ble-laptop-keyboard /tmp/keyboard-step1-20261006/clean-project
git -C /tmp/keyboard-step1-20261006/clean-project apply --index /tmp/keyboard-step1-20261006/candidate.patch
# Az explicit új fájlok másolása és a másolat indexébe felvétele után:
cd /tmp/keyboard-step1-20261006/clean-project
git submodule update --init --recursive lib/CD74HC4067 lib/ESP32-BLE-Keyboard-HUN
PLATFORMIO_CORE_DIR=/tmp/keyboard-step1-20261006/clean-core /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
PLATFORMIO_CORE_DIR=/tmp/keyboard-step1-20261006/clean-core /tmp/keyboard-step1-20261006/venv/bin/pio pkg list -e esp32c3
git submodule status
```

Mindegyik ellenőrzés **exit 0**, a tiszta build **SUCCESS, 49,28 s**. A két submodule újra az upstreamből klónozva a rögzített commitokra. A `clean-core` előtte nem létezett: a platform és minden szükséges buildcsomag újra letöltve; NimBLE a másolat új .pio/libdeps könyvtárába telepítve. A venv Core 6.2.0 azonos, a projekt-, build-, libdeps- és csomagkönyvtár izolált. A `pkg list` és a submodule státusz a fenti verziótáblával egyező eredményt adott. Nem használtuk a referenciaprojektek könyvtárait vagy buildjeit.

| Ellenőrzés | Célprojekt, izolált útvonalak | Tiszta másolat, új csomagtár |
| --- | --- | --- |
| Fordítás | sikeres, exit 0 | sikeres, exit 0 |
| Flash (PIO size) | 508300 / 1027200 byte, 49,5% | 507988 / 1027200 byte, 49,5% |
| Statikus RAM | 25548 / 327680 byte, 7,8% | 25548 / 327680 byte, 7,8% |
| firmware.bin fájlméret | 533552 byte | 533248 byte |
| firmware.bin SHA256 | dbd431c57db415b2debc8e4b637a5103bd2ff7ca1d23b85e5f7a7b5015aebb2d | 31abbeea2e45a6efa9e64e1bc6c0877537ae879ffb14ace5029bf93241639d8a |

**Nem bitazonos build:** az ELF szekcióméretek szerint a teljes 312 byte flash-különbség a `.flash.rodata` részben van (63728 → 63416 byte), a `.flash.text`, `.iram0.text`, `.dram0.data` és `.dram0.bss` mérete azonos. A két firmware `strings` kimenete eltérő beágyazott forrásútvonalakat mutat: a NimBLE fájlok egyikben `/tmp/.../libdeps`, másikban `.pio/libdeps`, a framework egyikben `core`, másikban `clean-core` útvonalú. Ez az eltérés magyarázatát alátámasztja, de teljes gépi kódekvivalencia-vizsgálat nem történt. A most igazolt reprodukálhatóság a rögzített deklarációkból, kézi pótlás nélkül megismételhető fordítás. A byteazonos image előállítása nem lett külön célként implementálva.

A tiszta inputban ellenőrizve nem létezik `src/secret.h`, `lib/ElegantOTA` vagy `AUDIT.md`. A forkok munkafája tiszta. A végső célprojekt és a tiszta másolat forrás/config/builddeklarációi byteonként egyeznek (a folyamatosan bővülő PROGRESS.md kivételével). A sikeres buildek újrafuttatása nem történt; az eltérő méretet read-only ELF/strings ellenőrzéssel vizsgáltuk.

## Korlátok és későbbre hagyott munka

Az audit reconnect/rollover/modifier/debounce/callback-pointer/diagnosztika javításai nem részei az 1/8-nak. A board preset/module kapcsolat fent dokumentált; a konkrét flashváltozat és feltöltési paraméterek hardveren még ellenőrizendők. Nincs automatikus pinout/partíciócsere.

## Végső változáslista és elfogadási állapot

- Módosított: `.gitmodules`, `platformio.ini`, `src/main.cpp`, `README.md`, `docker/Dockerfile`, `docker-compose.yml`, `.github/workflows/platformio.yml`.
- Új: `requirements-build.txt`, `PROGRESS.md`.
- Törölt követett gitlink: `lib/ElegantOTA`. Megtartott, inicializált és változatlan commitú gitlink: `lib/CD74HC4067`, `lib/ESP32-BLE-Keyboard-HUN`.
- Megőrizve: `AUDIT.md` (SHA256 854dac7ba606a04b466568a4b690e329cd100f006eec2923be24be5ab3c3c1b2), a pinout, teljes mátrix/kiosztás, jelenlegi BLE billentyűkezelés és `custom.csv`.
- `git diff --check` és `git diff --cached --check`: exit 0. A három referencia munkafája a végén is tiszta. A célprojekten kívül csak /tmp ellenőrzési anyagok készültek.
- A helyi sikeres BLE-build és a tiszta deklarációból sikeres build elfogadási feltétele teljesült. OTA/secret/webes könyvtár nélkül, megtartott Arduino2/NimBLE1 API-val. Docker/CI futtatás, hardver és hostpróba nem történt.
- Commit/push/publikálás/feltöltés/flash- vagy NVS-törlés nem történt. A célprojektben csak az OTA gitlink törlése stage-elt; a többi változás review-ra hagyott munkafaváltozás, az új fájlok még untrackedek. Egy későbbi commitnak a requirements-build.txt-t és PROGRESS.md-t is tartalmaznia kell; az audit már korábban untracked volt.

## Folytatási ellenőrzőpont

**Elkészült:** az 1/8. lépés rekonstruált, OTA nélküli BLE-kiindulópontja; két sikeres C3 build (második tiszta másolatból és üres csomagtárból), rögzített verziólista, méretek és forráslinkek, megőrzési és végső Git-ellenőrzés. **Ebben a lépésben nincs hátralévő implementáció.** A 2/8 nem kezdődött el; továbblépés külön felhasználói kérésre. A runtime hibák, reconnect-szabály és host/hardver-ellenőrzés a későbbi lépésekhez maradtak.

Megszakadás/új session után: először a jelenlegi Git diff és ez a PROGRESS.md olvasandó; nem az AUDIT eredeti secret/OTA buildfeltételeit kell újra létrehozni. Az átmeneti naplók/binaryk `/tmp/keyboard-step1-20261006` alatt vannak (`baseline-build-network.log`, `clean-build.log`, `build/esp32c3`, `clean-project/.pio/build/esp32c3`, `python-packages.txt`). A /tmp tartós megőrzése nem garantált; a README és a követett pinek elegendők egy új fordításhoz. Ne kezdd el automatikusan a 2/8-at, és ne töltsd fel a baseline-t a teszthardverre.


# Felújítás — 2/8: biztonságos diagnosztika

Dátum: 2026-10-07 (Europe/Budapest). **Állapot: 2/8 kész; 3/8 nem kezdődött el.** Az előző 1/8. szakasz történeti eredményeit változatlanul megőrizzük.

## Kiinduló állapot és döntés

Alkalmazható AGENTS.md nem található. A teljes unstaged és staged diff ellenőrizve: az 1/8. változásai vannak jelen; stage-elve csak az ElegantOTA gitlink törlése. AUDIT.md, PROGRESS.md és requirements-build.txt továbbra is untracked. A CD74HC4067 és HUN submodule a rögzített commiton, tiszta munkafával. A pinout, mátrix, HU kiosztás, report/reconnect és baseline-függőségek megmaradnak. Csak az F06 adat-élettartam javítása és a megfigyelést szolgáló hookok/logritkítás engedélyezett.

A tartós javítás követett patch lesz a HUN rögzített upstreamjéhez; a build generált külön példányra alkalmazza. Nem lesz kizárólag cache-ben vagy dirty submodule-ban élő javítás. NimBLE 1.4.3 szerint onAuthenticationComplete, onSubscribe és onStatus elérhető; notify void, disconnect-reason a szervercallbackben nem elérhető. Nincs stackmigráció vagy naplószint-emelés.

## Folytatási ellenőrzőpont — 2/8

**Elkészült:** audit/progress/diff/submodule áttekintés, baseline-pillanatkép /tmp/keyboard-step2-20261007 alatt, verzióhoz illő callback/UART források olvasása. **Következő:** F06 helper+patch és minimális üres/egybyte-os ellenőrzés, majd generált könyvtár bekötése és szinkronizált, korlátos diagnosztika. **Hátra:** build, tiszta másolat egyszeri buildje, méretdelta, naplógyűjtési leírás, végső ellenőrzés. 3/8 nem indul el.

**F06 ellenőrzőpont:** a követett KeyboardLedValue.h helper helyi értékben tartja a getValue() eredményét és ürességellenőrzés után másolja a byte-ot. A tools/check_led.cpp ugyanazt a helpert hívja érték szerint visszaadó karakterisztikahelyettessel: üres, egybyte-os 0x80 és 0x00 eset. GCC C++11, -Wall/-Wextra/-Werror és AddressSanitizer/UBSan. Az első futás exit 1 volt a sandboxban nem működő LeakSanitizer/ptrace miatt; detect_leaks=0-val az ASan/UBSan ellenőrzés exit 0. Ez nem valódi BLE-host LED-írás tesztje. A diagnosztikai callback-hookok csak ezután készülnek.

**Build-ellenőrzőpont:** az első 2/8 build SUCCESS, exit 0 (10,19 s). Flash 510984 byte, statikus RAM 26044 byte. A gráf NimBLE 1.4.3 / Project patched HUN BLE Keyboard 0.3.3 / CD74HC4067 1.0.2; az eredeti HUN submodule továbbra is tiszta. Az eredeti könyvtár neve lib_ignore-ban kizárt, a pre-build script SHA256-manifest szerint ellenőrzi az upstream fájlokat, külön generált példányra alkalmazza a követett patch-et, és csak ezt fordítja. A callbackek 16 elemű, portMUX-védett sorba írnak; nincs callback Serial-kiírás. A heartbeat kapcsolattól független, a korábbi 10 ms / 1000 ms várakozások megmaradtak. **Következő:** naplógyűjtő leírás lezárása és a tiszta másolatból egyszeri reprodukció.

**Reprodukciós ellenőrzőpont:** a tiszta másolat /tmp/keyboard-step2-20261007/clean-project; HEAD clone + teljes Git patch + explicit új projektfájlok a másolat indexébe felvéve. Candidate tree: 6a8b37bd805bbe14539dbfa3533017b86ea1016b. Régi .pio, generált HUN példány, secret vagy szomszédos referencia nem másolt. A két deklarált upstream submodule új letöltésből inicializálódik; a baseline Core/tool/framework és NimBLE pontos cache-e újrahasznált, frissítés nélkül. Tiszta build napló: /tmp/keyboard-step2-20261007/clean-build.log. **Következő:** futó tiszta build eredményének olvasása, diag mezők/API-korlátok és elfogadási állapot végleges dokumentálása; sikeres buildet ne ismételj.

**API-pontosítás / új kód:** a fenti kezdeti „disconnect-reason nem elérhető” megállapítás csak a szervercallbackre igaz. A NimBLEDevice::setCustomGapHandler 1.4.3-ban nyilvános passzív event-listenert regisztrál; a nyers BLE_GAP_EVENT_DISCONNECT.reason és ENC_CHANGE.status átadható a korlátos diagnosztikai sornak. A listener visszatérését az upstream nem használja policyhoz. Egyszer regisztrálunk az onStarted ponton, init után és advertising előtt. A tiszta másolatból végzett patch-reprodukció már SUCCESS (9,39 s, 510984 flash / 26044 RAM). Az ezt követő GAP-megfigyelő forrásváltozása miatt egy új célbuild szükséges, a patch tárolása/bekötése nem változott; a tiszta buildet nem ismételjük.

## Végleges 2/8 változások és tartós könyvtárjavítás

- `patches/ble-keyboard.patch`, `KeyboardLedValue.h`, `hun-upstream.json`, `patches/README.md`: F06 javítás + a megfigyeléshez szükséges input-report callback/hookok. A LED-érték helyi tulajdonolt objektumban marad a hosszellenőrzés és byte-másolás végéig; üres értékből nincs olvasás. A korábbi callback ESP_LOGI kiírása helyett csak scalar LED-esemény keletkezik, soros kiírás nélkül.
- `scripts/prepare_build.py`, `platformio.ini`: hash-ellenőrzött upstreamből minden buildben generált külön HUN példány; erre kerül a követett patch/helper. A HUN gitlink és `.gitmodules` az 1/8-hoz képest változatlan, a submodule nem dirty. Az eredeti `ESP32C3 BLE Keyboard` név `lib_ignore` alatt kizárt, csak a generált `Project patched HUN BLE Keyboard` fordul. A buildgráf ezt igazolja, nincs második aktív HUN forrás. A script hiányzó/eltérő upstream fájl esetén leáll, nem írja felül a helyi upstream változtatásokat. A buildgéphez GNU patch szükséges; a Core/firmware függőségek nem frissültek.
- `include/Diagnostics.h`, `src/Diagnostics.cpp`: megfigyelő subclass, 16 elemű fix eseménysor, számlálók és időmérés; rövid portMUX critical section védi a callback/loop adatátadást. Csak skalár mezők másolódnak; callbackből nem kerül soros kiírás, megőrzött pointer, String vagy dinamikus esemény a loophoz. A callbackek nem módosítják a HID/report/mátrix állapotot. A GAP listener egyszer, init után/advertising előtt regisztrált, nem végez stack resetet, nem szűri vagy módosítja az eseménypolicyt.
- `src/main.cpp`: mérés az eredeti scan és loop körül, API-visszatérések számlálása; billentyűnkénti és ismétlődő várakozó kiírások kikapcsolva. A pinout/mátrix/HU-map, sorolvasás/GPIO setup, 20 µs settling, 10 ms aktív és 1000 ms disconnected várakozás, connection-gate és press/release hívások maradtak. Az esemény eredményét csak számoljuk; nincs retry, releaseAll, új debounce, readiness-gate vagy reconnect policy.
- A soros TX-ring fix 1024 byte, a formatter fix 384 byte stack-buffer. A logger csak elegendő `availableForWrite()` mellett ír, legfeljebb egy eseménysort/ciklust és két heartbeat-sort/10 s; nincs flush, retry, új sleep vagy végtelen sor. Telítettség/formatkorlát esetén az output eldobódik és számlálódik. A UART driver meglévő belső lockjait továbbra is használja; a valós késleltetés hardveren nem mért. Az 1024 byte driver-ring dinamikus heapfogyasztása nincs benne a statikus RAM-méretben.
- `bin/recv.sh`, `bin/bbread.py`, `README.md`: egységes 115200 baud, explicit port; a Python nyers és UTC receive-timestamp mellékletet ment 256 byte-os chunkokkal. A firmware baud/monitor már 115200 volt és maradt. A collector nem nyitott portot az ellenőrzés során; DTR/RTS lekapcsolását open előtt állítja, a driver/adapter esetleges impulzusa hardveren még ellenőrizendő.

**Upstream és licenc:** a teljes eredeti HUN tree forrásazonosítója `5886c43812b81303501017aa39f91d6e775bf93e`, SHA256-manifesttel; README/credits/metadata/examples sértetlenül bekerül a generált példányba. A rögzített upstream tree nem tartalmaz LICENSE/COPYING fájlt vagy egyértelmű teljes könyvtárlicencet. Nem távolítottunk el licencet és nem adtunk hozzá kitalált MIT/GPL besorolást. A felsőbb upstream [szerzői licencbeszélgetése](https://github.com/T-vK/ESP32-BLE-Keyboard/issues/60) nem rendezi automatikusan a HUN fork egészét. Ez nyitott publikálási kérdés; a mostani megoldás megtartja az eredeti forrást és attribúciót, a változásokat követett patchben tárolja. A referenciarepo változatlan.

## Elérhető diagnosztikai adatok és pontos értelmezés

**Az összes runtime esemény és idő/heap-adat hardveren ellenőrizetlen.** A build és a célzott LED-helper teszt nem szimulál valódi BLE hostot vagy resetet. Nincs ebben a körben rögzített valódi BOOT/HB/BLE napló.

| Sor / mező | Ténylegesen megfigyelt adat, ha a hardveren lefut | Korlát |
| --- | --- | --- |
| `BOOT id`, `built_utc` | HEAD + dirty/clean/unknown + 12 hex tartalomfingerprint és fordítás UTC; generált header/JSON | A dirty állapot staged, unstaged, untracked és submodule változásokat is lát; nem csak HEAD. Git nélküli export unknown. A fingerprint forrás/include/script/patch/CD-fájlokat és ini/partíció/requirements deklarációkat hash-el, néhány metadata/README fájlt is konzervatívan tartalmaz; nem teljes hermetikus lock vagy biztonsági aláírás. |
| `BOOT reset=N/name`, `idf` | `esp_reset_reason()` nyers enum + név; `esp_get_idf_version()` | Resetok nem bizonyít önmagában konkrét hibás kódutat. Watchdog/panic besoroláshoz a teljes nyers panic/backtrace napló is kell. |
| `HB t_ms` | 64 bites esp_timer uptime, loopból kb. 10 s-onként | Nem külön feladat: ha a loop megáll, nem jön heartbeat. Disconnected állapotban az eredeti 1 s loop-delay miatt kb. 1 s jitter lehet. |
| `loop_us`, `loop_max_us` | Utolsó befejezett teljes loop és boot óta legnagyobb idő, az eredeti delay és logger költségével | A heartbeat az előző befejezett loopot mutatja; a folyamatban megakadt ciklus még nem kerül maximumba. |
| `scan_us`, `scan_max_us`, `scans` | A teljes writeRows scan ideje, benne az eredeti per-key BLE hívások; eddigi maximum és számláló | Bontás alatt továbbra sincs scan. A régi utolsó scan marad kiírva; nulla scans esetén még nincs megfigyelt scan. Ez nem tisztán GPIO-idő. |
| `heap`, `heap_min` | Aktuális és boot óta minimum malloc-képes szabad heap, byte | Nem legnagyobb összefüggő blokk és nem önmagában szivárgásbizonyíték. |
| `BLE connect/disconnect`, `handle`, `link_obs` | Server callbackből descriptor handle; link_obs a legutóbbi megfigyelt callback állapota | Nem HID-ready vagy hostkapcsolati garancia, nem per-host állapottábla. A régi connected bool/gate nincs kijavítva. A server disconnect `code=-1` jelentése: ezen a callbacken nincs okadat. |
| `BLE gap_disconnect code` | GAP listenerből nyers `event.disconnect.reason`, ugyanahhoz a handle-höz | Valódi stack-okadat; nem következtetünk belőle mérés nélkül táp- vagy RF-hibára. A sima disconnect és GAP esemény két külön megfigyelés. |
| `BLE encryption code` | `ENC_CHANGE.status`, nyers stackkód, handle | 0 a titkosított állapotváltoztatás stackeredménye; nem általános pairing/hostkompatibilitási siker. |
| `BLE auth value` | Security-descriptor bitmask: bit0 encrypted, bit1 authenticated, bit2 bonded | Nem írunk „auth siker” állítást csak a callback neve miatt. A sikertelen titkosítás nyers státusza a külön encryption eseményben lehet megfigyelhető. |
| `BLE subscribe report/value` | Report 1 keyboard / 2 media, bit0 notification / bit1 indication; 0 unsubscribe | Csak az aktuális subscribe-esemény, nem új readiness vagy küldési policy. |
| `TX notify_attempts`, `skipped_disconnected` | sendReport belépési hook: connected ágon megkísérelt notify út / bontott gate miatt kihagyott sendReport | A hook setValue/notify előtt fut; ez **küldési kísérlet**, nem notify return value vagy hostvétel. A notify 1.4.3-ban void. |
| `TX status_ok/error`, `last_status/code` | onStatus callback által jelentett enum és nyers kód; sikeres státuszok aggregáltak, hibák korlátos eventként is látszanak | Ez **helyi stackvisszajelzés**. A notification nem host HID-feldolgozási ACK; státusz nélküli csend nem sorolható automatikusan küldési hibának. |
| `key_api_ok/zero` | press/release nemnulla/nulla visszatérése a hívás után | Lokális billentyűkönyvtár eredménye, nem delivery; a nullacellák/rollover/modifier auditproblémák is beleszámolhatnak. Nem írjuk ki a gépelt billentyűket. |
| `BLE led value` | Biztonságosan másolt LED byte; -1 az üres érték jelzése | LED-output hostírás, nem billentyű-input átvételi visszaigazolás. A handle itt és status_error-nál 65535 (API-n nem elérhető); lifecycle report=0. |
| `ev_drop`, `log_drop` | Fix sor telítődése miatt eldobott események / UART helyhiány vagy formatkorlát miatt eldobott sorok | Nem tekintjük teljes eventtrace-nek a naplót. Számlálók UINT32_MAX-nál telítődnek, nem növekvő memóriát használnak. last_status=-1 esetén még nincs státuszmegfigyelés. |

A 1.4.3 `notify()` feliratkozó nélkül csendben visszatér, security/MTU ágakon is kihagyhat küldést, és a notification ág közvetlen `ble_gattc_notify_custom()` return code-ját nem adja az alkalmazásnak. A `onStatus` a későbbi NOTIFY_TX eseményből dolgozik. Ezért az attempts és status számlálók nem feltétlenül 1:1 párok; nincs kitalált „sent/received” eredmény vagy nagy stackpatch. **Hostoldali vétel ebben a firmware-ben nem megfigyelt.**

Az eredeti callback/loop connected bool szinkronizációs és HID/reconnect/rollover hibák változatlanok. A diagnosztikai adatátadás külön szinkronizált, nem új használhatósági state machine. A bontáson át tartott gomb felengedés+új lenyomás szabálya továbbra is későbbi implementáció.

## Verzióhoz illő elsődleges források

- Resetok, SDK-verzió és heap API-k: [ESP-IDF 4.4.3 / C3 system](https://docs.espressif.com/projects/esp-idf/en/v4.4.3/esp32c3/api-reference/system/system.html). Uptime: [4.4.3 esp_timer](https://docs.espressif.com/projects/esp-idf/en/v4.4.3/esp32c3/api-reference/system/esp_timer.html). Ezek a baseline beépített IDF-jéhez illenek, nem stable/latest SDK következtetések.
- Panic/watchdog/brownout napló és ELF-hez kötött elemzés: [4.4.3 C3 fatal errors](https://docs.espressif.com/projects/esp-idf/en/v4.4.3/esp32c3/api-guides/fatal-errors.html). Gyors tápesés csonka naplót okozhat; a fizikai táp okának bizonyításához mérés kell. Heartbeat hiányából nem következik biztosan fagyás: soros kapcsolat/collector is kieshet.
- A használt release: [NimBLE 1.4.3](https://github.com/h2zero/NimBLE-Arduino/tree/1.4.3). LED-érték ownership: [NimBLEAttValue](https://github.com/h2zero/NimBLE-Arduino/blob/1.4.3/src/NimBLEAttValue.h); lifecycle/security API: [NimBLEServer.h](https://github.com/h2zero/NimBLE-Arduino/blob/1.4.3/src/NimBLEServer.h).
- `notify` void, subscribe és csendes kihagyás: [NimBLECharacteristic.cpp](https://github.com/h2zero/NimBLE-Arduino/blob/1.4.3/src/NimBLECharacteristic.cpp); NOTIFY_TX → onStatus és a régi connect/disconnect callbackek együtt hívása: [NimBLEServer.cpp](https://github.com/h2zero/NimBLE-Arduino/blob/1.4.3/src/NimBLEServer.cpp).
- A passzív nyilvános GAP listener: [NimBLEDevice.cpp](https://github.com/h2zero/NimBLE-Arduino/blob/1.4.3/src/NimBLEDevice.cpp#L1199), [ble_gap.c](https://github.com/h2zero/NimBLE-Arduino/blob/1.4.3/src/nimble/nimble/host/src/ble_gap.c#L6551). A register egyszerű listener-regisztráció, nincs külön task/allokáció; a dispatcher a listener return értékét nem használja policyként. Egyes webes source-nézetek cache-hibát adtak; az azonos pinned Registry-csomag helyi forrása olvasva/fordítva is ellenőrzött.
- UART kapacitás/write/ring implementáció: [Arduino-ESP32 2.0.6 uart](https://github.com/espressif/arduino-esp32/blob/2.0.6/cores/esp32/esp32-hal-uart.c), [HardwareSerial](https://github.com/espressif/arduino-esp32/blob/2.0.6/cores/esp32/HardwareSerial.cpp). A hook és eredeti könyvtár kizárása: [PlatformIO extra_scripts](https://docs.platformio.org/en/latest/projectconf/sections/env/options/advanced/extra_scripts.html), [lib_ignore](https://docs.platformio.org/en/latest/projectconf/sections/env/options/library/lib_ignore.html); a Core 6.2.0 helyi LDF LIBSOURCE_DIRS kezelésével együtt ellenőrzött.

## Ellenőrzések és méretek

| Ellenőrzés | Eredmény |
| --- | --- |
| LED-helper: g++ 15.2.0, C++11, warnings-as-errors, ASan/UBSan | Üres és egybyte-os 0x80/0x00 bemenet PASS, exit 0. LeakSanitizer a sandbox ptrace korlátja miatt külön kikapcsolva; nincs általános tesztkeretrendszer. |
| Első célbuild | SUCCESS, exit 0, 10,19 s; 510984 flash / 26044 statikus RAM; GAP listener előtt. |
| Egyszeri tiszta patch-reprodukció | SUCCESS, exit 0, 9,39 s; ugyanaz a patch/helper/buildhook, friss upstream submodule-ok és új generált könyvtár; 510984 flash / 26044 RAM. A később hozzáadott GAP appmegfigyelő ebben a buildben még nem volt. |
| Végső célbuild az új GAP appkód után | **SUCCESS, exit 0, 3,01 s; 511270 / 1027200 flash (49,8%), 26060 / 327680 RAM (8,0%).** Nincs C/C++ warning/error a naplóban. A könyvtárpatch/bekötés nem változott a tiszta bizonyítás után. |
| 1/8 helyi baseline-hoz viszonyított delta | 508300 → 511270 flash: **+2970 byte**; 25548 → 26060 RAM: **+512 byte**. Azonos Core/framework/NimBLE útvonalakat használtunk. |
| 1/8 tiszta baseline-hoz viszonyított delta | 507988 → 511270 flash: **+3282 byte**; RAM **+512 byte**. A baseline két flashértékének korábban dokumentált 312 byte útvonaleltérése miatt mindkét viszonyítás megadva. |
| Shell/Python collector | bash -n, Python py_compile és --help exit 0; port nem nyílt. Valódi capture, DTR/RTS, hostok és runtime diagnosztika **ellenőrizetlen**. |
| Megőrzési diff | Pin/mux/globális hardverdeklarációk, teljes HU mátrix, writeRows/pinSetup, custom.csv változatlan; press/release egyszeri hívások és delay-ek megmaradtak. Submodule-ok tiszták; referenciarepo-k végén is tiszták. |

A buildparancs a célprojekt gyökerében:

```sh
PLATFORMIO_CORE_DIR=/tmp/keyboard-step1-20261006/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step2-20261007/build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step1-20261006/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
```

Tiszta reprodukció: HEAD helyi clone + `git diff HEAD --binary` patch + explicit új projektfájlok a másolat indexében. Új upstream `git submodule update --init --recursive`, majd `/tmp/keyboard-step2-20261007/clean-project` alatt ugyanaz a Core/libdeps, alapértelmezett friss .pio builddir. Input tree `6a8b37bd805bbe14539dbfa3533017b86ea1016b` a GAP appbővítés előtti, köztes PROGRESS-szel. Nem másoltunk régi patched könyvtárat vagy secretet. Letöltés csak a két rögzített upstream submodule tiszta klónozásához történt; a baseline platform/framework/tools/NimBLE cache újrahasznált, függőségfrissítés nélkül.

Pontos változatlan buildverziók: PlatformIO Core 6.2.0; espressif32 6.0.1; framework 3.20006.221224 = Arduino 2.0.6 / IDF 4.4.3; RISC-V 8.4.0+2021r2-patch5; esptool 1.40400.0; SCons 4.41101.0; NimBLE 1.4.3; CD gitlink aa6959c25a9ad7160fd515f73d8727aa86fc1ffd; HUN gitlink 5886c43812b81303501017aa39f91d6e775bf93e + a projekt követett patch-e, metadata 0.3.3. Nincs új BLE stack/framework vagy frissességi audit.

Végső buildazonosító: `54adbb04939d-dirty-src26994af33642`, UTC `2026-10-07T01:09:17Z`. firmware.bin fájlméret 537248 byte (nem azonos a PIO flash használatával). SHA256 bin: `003748399541f58b77de8a72ef12f8357ec8b64b5e8b8ceedeaaa571c6db0182`; ELF: `96be535d6382e124db9f1d3fbecfca797fa5d76bc8a6208a90bfdb1dadad9534`. A végső bin/ELF/JSON/log/SHA256SUMS pár `/tmp/keyboard-step2-20261007/final-artifacts` alatt együtt megőrizve, csak átmeneti helyen; most hardverre nem kerültek.

## Későbbi hardverpróba: rövid naplógyűjtési menet

1. Külön jóváhagyott próba előtt azonosítsd a soros adaptert/portot és a hardver flash/boot paramétereit. Az eredeti tesztfirmware-t most nem változtattuk meg; e diagnosztikai image feltöltése új feladat. Ne nyiss most portot vagy törölj NVS-t a diagnosztika igazolásának kedvéért.
2. Még a próba/rebuild előtt tartós, azonos mappába másold az **adott image** firmware.bin, firmware.elf és generated/build-info.json fájlját és build naplóját. A bin/ELF SHA256-ját és a reprodukálható source/patch állapotot is őrizd meg. A mostani /tmp csomag nem tartós archiválás.
3. Egyetlen collector az ellenőrzött porton, 115200 baud: `python bin/bbread.py PORT /tartos/naplomappa/serial.BFL`; a build venv tartalmaz PySerialt. A `.BFL` a változatlan nyers adat, `.BFL.utc.txt` host UTC vételi idő és chunkok (nem firmware eseményidő). Ctrl+C lezárja/flush-eli. Alternatíva Linuxon: `bash bin/recv.sh PORT LOGFILE`, nyers capture. A Python DTR/RTS open előtti beállítása mellett adapter/OS oldali resetimpulzus nem kizárt; a tesztmenetben figyeld a BOOT megjelenését. Most csak szintaxis/help ellenőrzés történt.
4. A tesztleírásban rögzítsd a host/OS-t, tápot, távolságot és a csatlakozás/bontás időpontját. Először normál bootot, connected és disconnected heartbeatet, majd host connect/auth/subscription/disconnect eseményeket keress. LED-írás üres és egybyte-os valódi GATT esetét később külön ellenőrizd. Ne változtasd meg emiatt a tartott gomb/reconnect vagy watchdog/brownout policyt.
5. Panic/watchdog/brownout esetén a teljes nyers naplót és az előző + következő BOOT sort is őrizd meg az **azonos ELF-fel**. Heartbeat eltűnése plusz BOOT hiánya nem különíti el biztosan a loopmegakadást és a soros kiesést. Brownout resetjelzés támogatja a tápesemény vizsgálatát, a fizikai okot mérésnek kell igazolnia. Hostoldali karaktervételhez külön hostmegfigyelés kell, a notify_status_ok nem elég.

## Végső 2/8 változáslista és határok

Módosított ebben a lépésben: `src/main.cpp`, `platformio.ini`, `bin/recv.sh`, `bin/bbread.py`, `README.md`, `PROGRESS.md` (csak hozzáadott 2/8 szakasz). Új: `include/Diagnostics.h`, `src/Diagnostics.cpp`, `scripts/prepare_build.py`, `patches/ble-keyboard.patch`, `patches/KeyboardLedValue.h`, `patches/hun-upstream.json`, `patches/README.md`, `tools/check_led.cpp`.

Az 1/8 PROGRESS-rész byteonként megmaradt, AUDIT.md eredeti SHA256-ja ellenőrizve. A meglévő staged ElegantOTA törlés megmaradt; 2/8 nem stage-elt, új fájljai review-ra untrackedek. Ezeket egy későbbi projektcommitnak mind tartalmaznia kell, különösen a patch/helper/manifest/buildscript csomagot. A submodule-ok és `.gitmodules` az 1/8-hoz képest nem módosultak. Docker/CI/requirements/fork referencia nem módosult ebben a lépésben, nincs ismételt Docker/CI/ref build vagy dependency-audit. Nincs feltöltés, flash/NVS-törlés, watchdog/brownout/reset-policy csere, stack-helyreállítás, debounce/HID/reconnect javítás, OTA, push vagy publikálás.

## Folytatási ellenőrzőpont — 2/8 lezárva

**Elkészült:** F06 tartós patch és célzott LED-helper ellenőrzés; kis terhelésű, szinkronizált és korlátos diagnosztika; 115200 capture és bin/ELF együtt megőrzésének leírása; sikeres végső C3 build; egyszeri tiszta könyvtárpatch-reprodukció; API-korlátok és méretdelta dokumentálva. **Ebben a lépésben nincs hátralévő kódmunka.** Hardveren minden BOOT/HB/BLE/capture/timing/heap esemény ellenőrizetlen. A hiányzó egyértelmű HUN licenc publikálás előtt tisztázandó. **3/8 nem indult el.**

Új session esetén olvasd ezt a külön 2/8 szakaszt, a Git staged+unstaged diffet és patches/README.md-t. A legutolsó érvényes build napló `/tmp/keyboard-step2-20261007/build-final.log`, metadata/artifact `final-artifacts`, buildkönyvtár `build/esp32c3`. A `clean-build.log` a GAP observer előtti patch-reprodukció, nem a végső GAP-kód tesztje. Az első `build.log` szintén köztes. A /tmp megőrzése nem garantált; új build a követett fájlokkal és pinned submodule-okkal a README szerint készíthető, GNU patch szükséges. Ne ismételd a sikeres ellenőrzéseket új változás nélkül, ne tölts firmware-t, és ne kezdd el automatikusan a 3/8-at.

**Utolsó ellenőrzőpont:** a végső build tartalomfingerprintje megfelel a mostani forrás/script/patch állapotnak, és a dirty jelölés jelen van. Az egyszer tiszta másolatban bizonyított patch és prepare_build.py byteonként egyezik a végső változattal. Git staged lista változatlan (csak lib/ElegantOTA törlés), mindkét submodule tiszta; diff whitespace-check exit 0, audit hash és teljes 1/8 progress prefix sértetlen. Sikeres fordítás nem futott újra a lezáró read-only ellenőrzésekhez.

A célzott LED-ellenőrzés ismételhető parancsa (nem új tesztkeretrendszer):

```sh
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g tools/check_led.cpp -o /tmp/keyboard-check-led
ASAN_OPTIONS=detect_leaks=0 /tmp/keyboard-check-led
```

A detect_leaks=0 a jelen sandbox LeakSanitizer/ptrace korlátja miatt szerepel; ASan és UBSan aktív marad. A helper valódi firmware-típusra történő template-illeszkedését az ESP32-C3 build ellenőrizte, a BLE-host LED-írását nem. **Itt megállunk, 3/8 nem kezdődött el.**


# Felújítás — 3/8: fenntartható BLE HID

## Kiinduló ellenőrzés és visszaállítható pillanatkép

Alkalmazható AGENTS.md nincs a projektben vagy az ősútvonalakon. AUDIT és a lezárt 2/8 szakasz olvasva; teljes staged/unstaged diff és új fájlok ellenőrizve. A staged ElegantOTA-gitlink törlés megmarad. A CD és HUN submodule tiszta, rögzített commitjuk változatlan. A referenciák nem módosulnak.

**Migráció előtti, tartós helyi pillanatkép:** `.local-snapshots/step-2-20261007/` a célprojektben, helyi `.git/info/exclude` bejegyzéssel kizárva a Gitből. `source.tar.gz` mind a 41 forrás/config/dokumentáció/library fájlt tartalmazza, beleértve az untracked 2/8 patcheket és scripteket, a teljes PROGRESS/AUDIT szöveget és a két submodule forrását; nincs generált .pio. SHA256: `e90d1f9d123d3e9bb897e0785f771677aa76628a6aeece5b6aec492b3d5575fe`. `manifest.json` fájlonkénti SHA256, a tarból visszaolvasva ellenőrizve. `head.bundle` és a két név szerinti submodule-bundle őrzik az eredeti Git-történetet; staged/unstaged/candidate patch, index és státusz is mentve. Commit nem készült. A helyi mentés nem publikálandó.

Visszaállítás először **új üres könyvtárba**: `tar -xzf .local-snapshots/step-2-20261007/source.tar.gz -C UJ_KONYVTAR`, majd a manifest SHA256-jainak ellenőrzése. Ez önmagában a teljes 2/8 forrásállapot; Git-környezet rekonstruálásához a mentés README.txt és bundle/patch állományai használhatók. Újabb munkafát ne írj felül vakon. A mentés helyi lemezen tartós, nem kizárólag /tmp vagy régi HEAD.

## Folytatási ellenőrzőpont — 3/8 / feltárás

Elkészült: megőrzési ellenőrzés és migráció előtti mentés. Folyamatban: a három jelölt elsődleges forrás/API/licenc összevetése. Következő: döntés rögzítése, szükséges platformfrissítés külön buildje, majd csak a választott megoldás migrációja. Hátra: célzott LED/HID-egyezés, végső és egyszeri tiszta build, csomaglista/méret/dokumentáció. Hardverhez nem nyúltunk.

## Jelöltek összevetése és döntés

| Jelölt | Illeszkedés és fenntartási teher | Döntés |
| --- | --- | --- |
| HUN fork célzott NimBLE2-portja | A jelenlegi hat slot/8 modifier és magyar ASCII-map portolható Arduino2 alatt; HID-metódusok, összes callback, advertising és diagnosztikai patch átírása szükséges. Teljes report API külön hozzáadandó; 7 ms busy-wait, ordinary connected bool és modifier/telített-report hiba is megmaradna célzott javítás nélkül. A 2/8 snapshot `lib/ESP32-BLE-Keyboard-HUN/BleKeyboard.{h,cpp}` igazolja. A teljes fork licencengedélye nem tisztázott. | Nem választott: régi fork és saját patchréteg további fenntartása, publikálási akadály. |
| Kis projektbeli adapter / NimBLE | Nyers usage + 8 modifier bit + hat normál slot; közvetlen teljes 8 byte-os report API; saját, rövid callback/loop állapotátadás és a meglévő diagnosztika. A NimBLE HID/security/advertising/notify API-ját közvetlenül használjuk. Csak a szükséges keyboard szolgáltatás, nincs általános print/media/sleep keretréteg. | **Kiválasztott.** A specifikus laptopmátrixhoz kevés kód, külső HID-fork/patch nélkül; a NimBLE a protokollstack. |
| HijelHID_BLEKeyboard 0.5.0, commit 985f845b40374c1567a311ae81f8d7b326238b7c | Nyers billentyűk és módosítók, lifecycle/security API, Apache-2.0 licenc. README Arduino3.x-et, metadata legalább 3.3.7-et kér. Teljes report és karakterisztika private; a diagnosztikai hookokhoz hozzáférés/patch kellene. `_sendKeyReport` 50 ms priming delayt és legfeljebb 20 vTaskDelay-es retryt használ; callback/loop flagjei volatile-ok. Többlet media/sleep/reconnect-policy kód. | Nem választott: Arduino3 és saját adapter/patch is kellene a mostani célhoz; a mátrixnak szükségtelen belső policy. |

A Hijel Windows/Linux/Android támogatása és felsorolt hosttesztjei **upstream README állítások**, nem e projekt ellenőrzött eredményei. Egyetlen új megoldás készül; a két elvetett jelöltet nem implementáljuk/buildeljük. A saját új adapter nem veszi át a HUN/Hijel implementációját, ASCII-tábláját vagy reportdescriptor-forrását; az eredeti mátrix jelentését adatként, a HID usage-okat USB-specifikáció alapján rögzítjük. Nem adunk új licencet tisztázatlan eredetű régi kódhoz.

Elsődleges források: [NimBLE 2.5.1 támogatott MCU-k](https://github.com/h2zero/NimBLE-Arduino/blob/2.5.1/README.md), [pinned HID API](https://github.com/h2zero/NimBLE-Arduino/blob/2.5.1/src/NimBLEHIDDevice.h), [Server API](https://github.com/h2zero/NimBLE-Arduino/blob/2.5.1/src/NimBLEServer.h), [Characteristic API](https://github.com/h2zero/NimBLE-Arduino/blob/2.5.1/src/NimBLECharacteristic.h), [Hijel README](https://github.com/HijelHub/HijelHID_BLEKeyboard/blob/985f845b40374c1567a311ae81f8d7b326238b7c/README.md), [Hijel metadata](https://github.com/HijelHub/HijelHID_BLEKeyboard/blob/985f845b40374c1567a311ae81f8d7b326238b7c/library.properties), [Hijel küldés/állapot forrás](https://github.com/HijelHub/HijelHID_BLEKeyboard/blob/985f845b40374c1567a311ae81f8d7b326238b7c/src/HijelHID_BLEKeyboard.cpp#L1164), [Hijel header](https://github.com/HijelHub/HijelHID_BLEKeyboard/blob/985f845b40374c1567a311ae81f8d7b326238b7c/src/HijelHID_BLEKeyboard.h#L555), [Hijel LICENSE](https://github.com/HijelHub/HijelHID_BLEKeyboard/blob/985f845b40374c1567a311ae81f8d7b326238b7c/LICENSE).

## Platformfrissítés — külön ellenőrzési szakasz

Választás: espressif32 **6.13.0**, a platform hivatalos Arduino2 útján **Arduino 2.0.17 / IDF 4.4.7**. [Release](https://github.com/platformio/platform-espressif32/releases/tag/v6.13.0), [platform.json](https://github.com/platformio/platform-espressif32/blob/v6.13.0/platform.json). Ez frissíti a régi 2.0.6 frameworköt anélkül, hogy Arduino3 csomagolási migrációra lenne szükség; a platform ESP-IDF5.5.3/GCC14 adatai az ESP-IDF frameworkre vonatkoznak, nem erre az Arduino buildre. Registry `pio pkg show`: Arduino2 pontos csomag **3.20017.241212+sha.dcc1105b**, esptool **2.41100.0** (a 6.13 release esptool4.11 csomagja, nem a későbbi latest). A C3 toolchain **8.4.0+2021r2-patch5**, SCons **4.41101.0** és Core **6.2.0** marad. [Framework Registry](https://registry.platformio.org/tools/platformio/framework-arduinoespressif32), [esptool Registry](https://registry.platformio.org/tools/platformio/tool-esptoolpy).

Következő: külön köztes C3 build még NimBLE1.4.3/HUN2/8 diagnosztikával. Csak a siker után indul az adapter/NimBLE2 kódmigráció. Flash/partíció/pinek/feltöltési paraméterek változatlanok.

**Platform-ellenőrzőpont:** a külön köztes build **SUCCESS, exit 0, 28,01 s** a változatlan NimBLE1/HUN kóddal. Flash **537022**, statikus RAM **24660** byte; 2/8-hoz képest +25752 / −1400 byte. Napló `/tmp/keyboard-step3-20261007/platform-build.log`. Letöltve: platform6.13.0, framework3.20017.241212+sha.dcc1105b, esptool2.41100.0; az esptool a Python segédfüggőségeit is telepítette. RISC-V/SCons és NimBLE1 cache újrahasznált. A platformfrissítés így külön bizonyított, nem a BLE-migrációra következtetés.

**NimBLE forrásrögzítés:** a 2.5.1 tag tényleges Git commitja GitHub API alapján `5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb`. A végső dependency ezt a teljes commitot használja egyetlen upstream Git-forrásból, nem a régi generált .pio forrásra támaszkodik. A konkrét commit tartalmazza a ConnInfo-paraméteres onStatus callbacket is; a verzió nélküli Doxygen és webcache nézet helyett a pinned header/implementáció az irányadó. [2.5.1 tag](https://github.com/h2zero/NimBLE-Arduino/tree/2.5.1), [rögzített commit](https://github.com/h2zero/NimBLE-Arduino/tree/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb).

Következő: direkt adapter, nyers HID-mátrix és átvezetett diagnosztika. A régi HUN/patch/hook még megmarad a visszaállításhoz; csak az új adapter sikeres buildje után távolítjuk el az aktív projektből.

**Adapter-ellenőrzőpont:** az új, egyetlen aktív direkt adapter buildje **SUCCESS, exit 0, 8,80 s**, flash **512866**, statikus RAM **24420** byte. Gráf: csak NimBLE2.5.1+sha.5ae2c43f és CD1.0.2; HUN nem fordult, bár a régi hook még elkészítette az inaktív példányt. Napló `/tmp/keyboard-step3-20261007/migration-build.log`. Az upstream NimBLEClient.cpp három bool/enum warningot ad (nem az alkalmazáskód), ezt nem rejtjük el stackpatch vagy logflag segítségével.

A minimális native LED- és HID-ellenőrzés mindkettő exit0: g++15.2.0, C++11, -Wall/-Wextra/-Werror, ASan/UBSan, a sandbox LeakSanitizer korlátja miatt detect_leaks=0. LED üres/0x80/0x00 bemenet; mind a144 fizikai cella, press/release és bal/jobb módosító bitje ellenőrizve. A tools/step2_hid_expected.h fix numerikus elvárt adatot a tartós snapshot tényleges régi konstansaival és teljes ASCII-táblájával külön visszaellenőriztük: mind144 egyező; a régi mátrix nem igényelt implicit Shift/AltGr bitet. Hat normál slot mellett az összes modifier külön kezelése is ellenőrzött. Nem készült általános tesztkeret.

Most az új build sikere után eltávolítható az aktív projektből a HUN gitlink, régi patch/manifest és overlay hook. Megmarad a tartós 2/8 snapshot és a buildazonosító generálása; a LED-helper az include alá került. Következő: tisztítás utáni végső build és egyszeri tiszta reprodukció.

## Végleges kódszerkezet, működés és diagnosztika

- `include/KeyboardHid.h`, `src/KeyboardHid.cpp`: közvetlen NimBLE HID, reportID1/8 byte + LED-output. NimBLEServer/Characteristic callbackek, külön rövid portMUX-védett single-link snapshot; callback nem ír mátrixot/reportot és nem Serial-logol. Egy boot-élettartamú HID objektum és a stack saját szolgáltatásai, nincs lifecycle-reset, saját task, eseményvisszajátszás vagy korlátlan queue. A pre-build script már csak dirty/tartalom/build-UTC azonosítót készít; GNU patch nem szükséges.
- `include/KeyboardReport.h`: nyolc modifier bit és hat slot; `submitReport()` teljes állapotot és külön Disconnected/NotReady/LocalAccepted/LocalRejected eredményt kínál a későbbi reportbuildernek. `press/release` egyelőre átmeneti élalapú lokális módosítás. Az eredmény bool csak lokális reportkezelés, nem delivery. A scanner továbbra is connected gate mögött, 10/1000 ms delayekkel és 20 µs beállással működik; a GPIO/scanfüggvények és pinout byteonként változatlanok.
- `include/KeyboardLayout.h`: mind144 cella ekvivalens nyers page0x07 usage. HUN ékezetes kulcsok: ö27, ü2D, ó2E, ő2F, ú30, ű31, í32, é33, á34; Y1D/Z1C; „0”35; a régi **ASCII 0x2D (mínusz) → usage38**, nem usage2D. E0–E7 modifier → 1<<(usage−E0), old128–135 helyett. A megjelenő karaktert a host magyar kiosztása adja. Nullacella nem reportkey. Pause/Fn/egyéb korábban nem leképezett cellához nem adtunk új funkciót. [USB-IF Usage Tables 1.6, Keyboard/Keypad page](https://www.usb.org/sites/default/files/hut1_6.pdf), [USB HID1.11 report item specifikáció](https://www.usb.org/sites/default/files/hid1_11.pdf). A descriptor önállóan ezekből meghatározott adat, nem HUN/Hijel forrásmásolat.
- `include/KeyboardLedValue.h` az eredeti 2/8 saját helper tartalmát változatlanul viszi tovább. onWrite ConnInfo alapján valódi handle-t ad a diagnosztikának, az érték lokális ownership + lengthcheck után másolt. Üres =value−1, nincs dereferálás.
- `.gitmodules` és a Git-index: HUN gitlink eltávolítva az új build után; a már staged ElegantOTA-törlés megőrizve. A HUN working source és `patches/*` kikerült az aktív projektből; a `.git/modules` helyi történetét nem töröltük. Egyetlen BLE-dependency és CD gitlink marad, a snapshot teljes régi fork+patch+manifest+hook állapotot megőrzi.

**A migrációból adódó, külön dokumentált javulások:** ordinary connected bool helyett szinkronizált linkadat; a tényleges notify előfeltétele titkosítás és keyboard notification subscription; független modifierbit-kezelés telített hat slot mellett; nincs saját per-event7ms busy-wait vagy rejtett retry; a NimBLE2 advertising-restart explicit engedélyezett. A régi, mátrixban nem használt consumer report2 nem jött át. A fizikai report újraépítése/hetedik tartott gomb visszafogadása nincs megoldva. A gyors egymás utáni élek/backpressure miatti elvesztés hardveres vizsgálatot és későbbi küldéspolitikát igényel; a régi busy-wait elhagyása önmagában nem vételi garancia.

**Alap BLE életciklus/security:** egy host egyszerre (CONFIG_BT_NIMBLE_MAX_CONNECTIONS=1); az elfogadott három OS külön célhost. init → szolgáltatások → server.start → advertising.start; advertiseOnDisconnect(true) helyreállítja a NimBLE1 alapviselkedést. Az advertising-name explicit (NimBLE2 nem implicit reklámozza), keyboard appearance és HID UUID. Nincs új connection-param/TX-power vagy bootkori bond törlés. Bonding/MITM/SecureConnections továbbra is true/true/true; a régi és új stack default IO_NO_INPUT_OUTPUT marad. Ez a kért security flag, **nem igazolt MITM pairing**: az IO-képesség és host tényleges negotiation hardveren ellenőrizendő. A NimBLE saját REPEAT_PAIRING ága egy peer régi bondját lecserélheti; ez a pinned stackben már meglévő policy, nem alkalmazásoldali boot-törlés. [pinned NimBLEDevice security](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/src/NimBLEDevice.cpp), [pinned NimBLEServer lifecycle/security](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/src/NimBLEServer.cpp).

NimBLE1→2 NVS-bond formátum külön kérdés: az upstream egyszeri migrációs helperét nem futtatjuk bootkor, bondot nem törlünk, NVS-hez/hardverhez nem nyúltunk. A régi tesztfirmware tényleges stackje/bondadata nincs bizonyítva; későbbi feltöltés előtt az állapot és a visszaállítás lehetősége külön eldöntendő. Forrás: [pinned migrációs útmutató](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/docs/1.x_to2.x_migration_guide.md). A visszaállítható **forrás**snapshot nem jelenti a hardveres bondformátum automatikus visszaalakítását.

## Diagnosztikai adatok az új API-val

BOOT reset/build/dirty és ~10s HB uptime/loop/scan/max/heap/min/ev_drop/log_drop megmaradt, kapcsolat nélkül is; a 16 elemű kritikus szakaszos sor, 384 byte formatter és 1024 byte UART TX-ring változatlan. Nincs callback Serial, logszint-emelés, új várakozás vagy új feladat. Reset API most az IDF4.4.7-hez illik: [system](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32c3/api-reference/system/system.html); a korábbi reset policyt alkalmazásból nem módosítottuk. A frameworkcsere SDK-defaultjai eltérhetnek, a build nem reset-runtime teszt.

| Megfigyelés | Aktuális értelmezés és korlát |
| --- | --- |
| connect/disconnect | Valódi callback handle; disconnect.code már a szervercallback nyers oka. Nincs duplikált gap_disconnect. Nem fizikai RF/táp diagnózis. |
| auth/encryption/subscribe | Auth bit0 encrypted, bit1 authenticated, bit2 bonded; GAP ENC_CHANGE nyers status; keyboard report1 subscribe bit0 notify/bit1 indicate. Csak notification támogatott a küldési úton. |
| stack_init/advertising_start | begin összeredmény / kezdeti adv.start bool. Sikertelen init után nincs automatikus retry/reset; a loop heartbeat él. Belső init megakadásakor ez nem garantált. |
| advertising_complete | Nyers GAP reason; a sikeres advertising nem automatikusan host felfedezés. A server belső disconnect-restart returnjét nem adja külön callback, ezért nem állítunk restart-sikert. |
| notify_attempts | Csak a tényleges notify hívások; skipped_disconnected / skipped_not_ready előfeltételhiány miatt kimaradt hívások. |
| api_accepted/rejected | Egy explicit peernek és bytebuffernek végzett notify bool lokális eredménye, közvetlen stack-return út; **nem host HID-vétel**. A callbackből nyers kód is jöhet, nem biztos1:1 megfeleltetés. |
| status_ok/error, last_status_code | Notification onStatus:0 helyi siker, más kód hiba. Nincs régi1.x statusenum. ConnInfo-handle-t itt nem használunk: e commit NOTIFY_TX ága nem tölti fel; error handle65535 tudatosan „nem elérhető”. |
| key_api_ok/zero, led | Lokális usage/report módosítás / nem kezelt nullacella vagy telített slot; LED host-output biztonságosan másolt byte, nem input-ACK. |

Az explicit connHandle + 8 byte értesítés kerül a `ble_gattc_notify_custom` bool-result útjára; a karakterisztika általános, handle nélküli notify-jával összekeverve nem naplózunk sikert. A critical section **nem** fogja át a notify/GATT hívást. Check és notify közötti bontás továbbra is lehetséges: lokálisfalse/státuszhiba megfigyelhető, nincs saját várakozás/retry. A linkhandle reuse/generáció és elmaradt report pótlása 5/8 feladat. Forrás: [pinned notify implementáció](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/src/NimBLECharacteristic.cpp#L272), [pinned callback header](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/src/NimBLECharacteristic.h). **Minden runtime esemény hardveren ellenőrizetlen.**

**Licenc/eredet:** NimBLE fő C++ API és NimBLE-stack Apache-2.0, a csomag saját LICENSE és vendorizált alkomponens licencei megmaradnak; CD MIT licence a változatlan submodule-ban. [NimBLE LICENSE](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/LICENSE), [CD LICENSE](https://github.com/waspinator/CD74HC4067/blob/aa6959c25a9ad7160fd515f73d8727aa86fc1ffd/LICENSE). Az új adapter saját, e feladathoz írt implementáció; az aktív firmware nem HUN-forrást vagy HUN-patch-et fordít. A történeti mentés HUN forrására nem állítunk tisztázott licencet és nem publikáljuk. A régi eredet- és licenckérdés történeti dokumentálását megőriztük, a projekt egészének publikálási licencét most nem jelöltük ki. PnP VID/PID és fix80% battery a baseline örökölt adat, nem saját vendorengedély vagy mért akkuszint; későbbi publikálás előtt az azonosítókat is felül kell vizsgálni.

**Végső célbuild a legacy elemek eltávolítása után:** SUCCESS, exit0,6,88s; **512866 flash /24420 statikusRAM**. Az egyetlen aktív BLE gráf és a hook/library tisztítás igazolt. Következő: egyszeri tiszta input másolat, csomaglista/méret és megőrzési ellenőrzés; sikeres célbuildet indokolatlanul nem ismételjük.

## Ellenőrzések lezárása és pontos csomaglista

Az egyszeri tiszta másolat buildje **SUCCESS, exit0,9,51s**. Hely: `/tmp/keyboard-step3-20261007/clean-project`; input Git tree **76e2654a5ef899cf8f5f03a79269f92c62c005b6**. HEAD helyi clone + a teljes `git diff HEAD --binary` patch `git apply --index`-szel + minden explicit untracked projektfájl a másolat indexébe felvéve. Nem volt benne .pio, HUN, patches, secret vagy kézi könyvtár. Csak a deklarált CD gitlink inicializálódott upstreamből; NimBLE teljes commit a deklarált GitURL-ből, a PIO package/download cache felhasználható. A Core/framework/tool csomagok cache-e újrahasznált; nincs teljes újraletöltési igény. A végső és tiszta példány minden src/include/script/buildconfig fájlja byteonként egyező; a PROGRESS utólag csak eredménnyel bővül. Mindkét letöltött NimBLE Git HEAD `5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb`.

| Összetevő | Tényleges verzió/forrás | Állapot |
| --- | --- | --- |
| PlatformIO Core | 6.2.0; requirements-build.txt | Változatlan, /tmp/keyboard-step1-20261006/venv/bin/pio |
| espressif32 | 6.13.0 | Pontos platformpin |
| Arduino framework package | 3.20017.241212+sha.dcc1105b | Arduino2.0.17 / IDF4.4.7; package.json és ESP_IDF_VERSION header ellenőrizve |
| RISC-V toolchain | 8.4.0+2021r2-patch5 | Változatlan pontos pin |
| esptool package | 2.41100.0, esptool.py4.11.0 | A platform megfelelő pinned esptoolja; csak image-generálás, nem feltöltés |
| SCons package | 4.41101.0 | Változatlan pontos pin |
| NimBLE-Arduino | 2.5.1+sha.5ae2c43f; teljes Git commit 5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb | Egyetlen upstream lib_deps GitURL, nincs saját upstream patch |
| CD74HC4067 | 1.0.2 metadata; gitlink aa6959c25a9ad7160fd515f73d8727aa86fc1ffd | Változatlan, egyetlen aktív helyi submodule |
| HUN/Hijel/OTA | Nincs az aktív dependency gráfban | HUN a helyi2/8 snapshotban; Hijel nem telepített/buildelt |

`pio pkg list -e esp32c3` exit0 és package.json ellenőrzések egyeznek. A forrás- és csomagpinek deklaráltak; a Core/esptool Python segédcsomagjainak tényleges listája `python-packages.txt`-ben rögzítve. A teljes pip transzitív környezet nincs hermetikus lockkal újracsomagolva; ez nem bitazonos firmware-build garancia. A szükséges esptool telepítésekor csak annak segédcsomagjai kerültek fel, használatlan firmware-könyvtárat nem frissítettünk.

| Build / ellenőrzés | Flash (PIO size, byte) | Statikus RAM (byte) | Eredmény |
| --- | ---: | ---: | --- |
| Lezárt2/8 baseline | 511270 | 26060 | Történeti referencia |
| Külön platformfrissítés, régi BLE | 537022 | 24660 | SUCCESS28,01s |
| Direkt adapter, legacy cleanup előtt | 512866 | 24420 | SUCCESS8,80s; a cleanup előfeltétele |
| Végső célbuild, cleanup után | **512866 /1027200 (49,9%)** | **24420 /327680 (7,5%)** | SUCCESS6,88s |
| Egyszeri tiszta projektcopy | **512530 /1027200 (49,9%)** | **24420 /327680 (7,5%)** | SUCCESS9,51s |
| Végső vs2/8 delta | **+1596** | **−1640** | Eltérő framework/stack/config mellett, nem tisztán adapterköltség |
| Tiszta vs2/8 delta | **+1260** | **−1640** | Utvonalfüggő rodata-különbség is jelen van |
| Platformcsere után BLE-migráció delta | **−24156** | **−240** | Azonos új platform, eltérő BLE és single-host config |

A végső és tiszta build 336 byte eltérése teljes egészében `.flash.rodata` méretében van (67384→67048); text/IRAM/data/BSS és eh_frame méret azonos. `riscv32-esp-elf-size -A` és a két szekciólista read-only diffje ezt mutatja (diff exit1 az eltérő fájlok jelzése). A beágyazott upstream forrásútvonalak is különböznek; ez az eltérést alátámasztó megfigyelés, nem teljes gépikód-egyezés bizonyítása. **A reprodukálhatóság itt ugyanabból a deklarált forrásállapotból, kézi pótlás nélkül sikeres buildet jelent, nem bitazonos image-et.**

**Warning pontosítás:** a teljes migration/final/clean napló hat `-Wint-in-bool-context` upstream warningot tartalmaz: öt NimBLEClient.cpp és egy NimBLEServer.cpp előfordulás a BLE_HS_HCI_ERR makróban. A korábbi ellenőrzőpont „három” értéke részleges naplóolvasás volt. Nincs saját alkalmazáskód-warning vagy fordítási hiba; az upstream warningok nem lettek elhallgatva vagy saját stackpatch-csel javítva.

A C3 QIO SDK-config releváns brownout, interrupt/task watchdog és panic-print/reboot define-jai a2.0.6 és2.0.17 csomag között azonosak (brownout level7, IWDT300ms, TWDT5s/panic); külön statikus ellenőrzés. Nem változott alkalmazásoldali watchdog/brownout/reset policy. A runtime viselkedést ez nem bizonyítja.

Parancsok (projektgyökér; ténylegesen végrehajtva):

```sh
# Köztes platformbuild: NimBLE1 libdeps, még a migráció előtt.
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step3-20261007/platform-build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step1-20261006/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
# Új adapter első buildje, még a legacy cleanup előtt:
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step3-20261007/migration-build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step3-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
# Végső build a cleanup után:
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step3-20261007/final-build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step3-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step3-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio pkg list -e esp32c3
# Az explicit source/config másolat elkészítése után, clean-project alatt:
git submodule update --init --recursive
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
```

A target és tiszta build siker után nem lett újra megismételve. Unit-framework, reference-project build, Docker/CI vagy frissességi audit nem futott. A két célzott natív teszt parancsa a README-ben; nincs host/pairing/stability szimuláció.

## Tartós artifact és későbbi hardveres ellenőrzés

A végső target bin/ELF/JSON/buildnapló/csomaglista/ellenőrzési összesítő/SHA256SUMS együtt tartós helyen: `.local-snapshots/step-3-build-20261007/`, helyileg Gitből kizárva. Build ID **54adbb04939d-dirty-src5c8287748abb**, UTC **2026-10-07T09:55:55Z**. Bin fájlméret **538160** byte, SHA256 **967e43a30a9f67024146ba7ab8f50fa7cb9c3e9da87ca32f81a2aa70e97d7c39**; ELF SHA256 **fc451afb7dd76f1f4ef31a138f3229ab34d5e325e1feb0734953b094d308e87c**. A bin fájlméret nem azonos a PIO flash használatával. Naplók és clean input egy másolata /tmp/keyboard-step3-20261007 alatt is megvan; az önálló2/8 source mentés továbbra is érintetlen.

Hardveres próba későbbi külön feladat: az eredeti tesztfirmware most változatlan a hardveren. Igazolt port/flash/boot/NVS-terv után az adott bin/ELF/JSON párhoz 115200 nyers és UTC-s capture a változatlan bin/bbread.py segítségével. A 2/8 naplógyűjtési menete megmarad, de a TX/disconnect mezőket már a fenti3/8 táblázat szerint értelmezd. Windows/Linux/Android alatt külön pairing/encryption/subscription/LED/gyors press-release/vételek, reconnect és hosszú használat próbák szükségesek. Nem igazolt hosttámogatást vagy BLE-stabilitást dokumentálunk.

## Konkrét hátralévő feladatok — 4/8 és5/8

- **4/8:** kapcsolat nélkül is mintavételezhető fizikai mátrix és időalapú debounce; stabil fizikai állapotból teljes KeyboardReport; mind a8 modifier külön, determinisztikus6-key kiválasztás és hetedik tartott gomb visszafogadása a slot felszabadulásakor; ghosting-policy csak igazolt hardverigény alapján. A3/8 `changeKeyboardUsage` átmeneti élkezelő, nem e javítások pótléka. A submitReport API már megvan, de a scanner most még connected gate mögött áll.
- **5/8:** connection/readiness-generáció átadása a loopnak, új generációnál függő report és régi állapot tudatos kezelése; a bontáson át tartott gomb **csak felengedés+új lenyomás után** aktív. Titkosítás/subscription előtt keletkezett elvesző report, helyi notifyfalse/backpressure és handle reuse rendezése, korlátos küldéspolitikával. Most nincs automatikus üres reconnect report, releaseAll vagy suppress-until-release. Az adapter readiness előfeltétele önmagában nem oldja ezt meg.
- Hardveren később: a valós flashváltozat/boot/pin+mux/táp/RF és hostok vizsgálata; a régi NVS/bondok migrációjáról külön döntés feltöltés előtt. A forrássnapshot nem firmware/NVS backup; a teszthardver eredeti binaryja most sem lett kinyerve vagy módosítva.

## Változáslista és folytatási ellenőrzőpont — 3/8 kész

Ebben a lépésben módosult: `.gitmodules`, `platformio.ini`, `src/main.cpp`, `include/Diagnostics.h`, `src/Diagnostics.cpp`, `scripts/prepare_build.py`, `tools/check_led.cpp`, `README.md`, `PROGRESS.md`. Új: `include/KeyboardHid.h`, `include/KeyboardReport.h`, `include/KeyboardLayout.h`, `include/KeyboardLedValue.h`, `src/KeyboardHid.cpp`, `tools/check_hid.cpp`, `tools/step2_hid_expected.h`. A LED-helper a régi saját helper áthelyezése. Törölve az aktív projektből: HUN gitlink/forrás és `patches/{ble-keyboard.patch,KeyboardLedValue.h,hun-upstream.json,README.md}`. Új, kizárólag helyi mentések: `.local-snapshots/step-2-20261007`, `.local-snapshots/step-3-build-20261007`; helyi `.git/info/exclude` bejegyzés. A régi HUN licence/attribúciója a2/8 teljes source archive-ban megmaradt.

**Elkészült:** tartós, teljes2/8 pillanatkép; három jelölt elsődleges forrásalapú összevetése és direkt adapter kiválasztása; külön platformbuild; NimBLE2 migráció és legacy cleanup; minimális LED+144-cellás HID megfeleltetés; sikeres végső C3 és egyszeri tiszta build; egzakt pinek, méretdelták és adatértelmezés. **3/8 implementációban nincs hátralévő munka.** Hardveren az összes lifecycle/notify/LED/BOOT/HB/host esemény ellenőrizetlen. A rekonstruált alap nem az eredeti firmware reprodukciója.

Új sessionben: először ezt a legutolsó3/8 szakaszt és a teljes staged/unstaged/untracked/submodule állapotot olvasd. A staged ElegantOTA törléshez csak HUN gitlink törlés járult; a többi módosítás és új fájl review-ra munkafában maradt, commit/push nem történt. Egy későbbi commitnak az új adapter/diagnosztika/buildscript/mapping/check fájlokat is tartalmaznia kell. A2/8 visszaállítás forrása a tartós helyi source.tar.gz, nem a régi HEAD; a mai build pontos naplója és bin/ELF párja a helyi step-3-build mentésben van. **4/8-at csak új kérésre kezdd el.** Most nincs feltöltés, portnyitás, flash/NVS-törlés, OTA, pinout/partíció/flashparaméter/TXpower változtatás, sleep/NKRO/új billentyűfunkció, publikálás vagy push.

**Utolsó lezáró ellenőrzőpont:** git diff --check és git diff --cached --check exit0. Stage-elve kizárólag ElegantOTA és HUN gitlink törlés; a többi munkafaváltozás megőrizve. A CD és mindhárom referenciarepo munkafája tiszta. Az AUDIT eredeti854dac7b… SHA256-ja, a teljes eredeti PROGRESS1/8–2/8 szöveg, a GPIO/scan/loop és a partíció/collectors/Core/CI/Docker fájlok byteonként megőrizve. A LED-helper a2/8-hoz képest byteazonos. A jelen forrás tartalomfingerprintje read-only újraszámolva **5c8287748abb**, egyezik a megőrzött végső és tiszta builddel; nincs indokolatlan újrafordítás. A TX-sor legrosszabb megadott számlálóértékekkel285byte, a384byte formatter alatt. A végső3/8 forrásokat a step-3-build mentés `source.tar.gz` és `source-manifest.json` állománya is megőrzi; a2/8 mentés ettől külön, változatlan. **Itt megállunk; a4/8 nem kezdődött el.**


# Felújítás — 4/8: stabil fizikai állapot és teljes HID-report

## Kiinduló ellenőrzés és megőrzés

Dátum: 2026-10-07. AGENTS.md nem található a célprojektben, a workspace-ben vagy a releváns ősútvonalakon. AUDIT és a lezárt 3/8 szakasz, teljes staged/unstaged diff, explicit untracked fájlok és submodule állapot áttekintve. A jelen forrás/dokumentáció byteonként egyezik a tartós `.local-snapshots/step-3-build-20261007/source.tar.gz` mentéssel. Mindhárom referenciarepo és az egyetlen CD submodule tiszta. Stage-elve változatlanul az ElegantOTA és HUN gitlink törlése. A meglévő helyi mentések 28 fájljának hash-e feljegyezve a megőrzés későbbi ellenőrzéséhez; egyik régi mentést sem írjuk át. Kiinduló státusz/diff/manifest `/tmp/keyboard-step4-20261007/` alatt, a tartós visszaállítás forrása továbbra is a 3/8 source archive.

## Választott működés

- A GPIO-kód csak teljes 8×18 nyers frame-et tölt; utána külön, loop által birtokolt KeyboardState végzi a per-cell timestamp alapú debounce-ot, majd a teljes stabil mátrixból reportot épít. A nyers és stabil mátrix const hozzáféréssel megmarad a későbbi5/8 részére. Nincs BLE-adapterfüggőség a feldolgozó kódban, új task vagy általános framework.
- **20 ms** közös press/release debounce-küszöb egy helyen, `include/KeyboardConfig.h`; **választott kezdőérték, nem hardveren bemért adat**. Minden nyers értékváltás csak az adott cella időbélyegét indítja újra. Stabil állapotváltás csak ugyanazon jel legalább20ms-nyi megfigyelt változatlansága után; unsigned32 időeltérés kezeli a millis wraparoundot. Kezdetben minden cella felengedett, a bootkor tartott gomb is végigmegy a késleltetett elfogadáson.
- Hat normál slot: a még stabilan lenyomott, előző kívánt reportban szereplő usage-ok **eredeti slotjukban maradnak**. A felszabadult helyeket a stabil mátrix sor–oszlop sorrendjében, egyedi usage-okkal töltjük fel. Telítettségkor új gomb nem váltja le a tartott kiválasztottat; üres hely esetén a korábban kimaradt, továbbra is tartott gomb új él nélkül bekerül. Az összes modifier külön OR-bitmező, nullacella kimarad, duplikált fizikai usage egy reportelem. Reserved és üres slotok0. Ez szándékos hatgombos kiválasztási szabály, nem NKRO/ErrorRollOver descriptor-migráció.
- Scan és feldolgozás BLE-kapcsolat nélkül is; az aktív ciklus eredeti `delay(10)` várakozása megmarad minden loop végén, a disconnected `delay(1000)` és gate kikerül. GPIO/mux/pinout és20µs beállás nem módosul. Egy scan után legfeljebb egy submitReport, csak a teljes kívánt report byteváltozásakor. A fizikai/kívánt állapot nem függ a notify eredményétől. Nincs offline élqueue, automatikus retry, kapcsolatgeneráció vagy reconnect-resync; változatlan állapotú sikertelen report újrapróbálása5/8 feladat.

Elsődleges háttér: [QMK debounce típusok](https://docs.qmk.fm/feature_debounce_type) (timestamp, szimmetrikus késleltetés, per-key függetlenség; nem veszünk át QMK kódot/függőséget), [USB HID1.11](https://www.usb.org/sites/default/files/hid1_11.pdf) és [USB Usage Tables1.6 Keyboard/Keypad](https://www.usb.org/sites/default/files/hut1_6.pdf) (modifier/report-formátum). A konkrét6-key prioritási szabály projektbeli döntés, nem a specifikáció kötelező sorrendje. [NimBLE2.5.1](https://github.com/h2zero/NimBLE-Arduino/tree/2.5.1) és a ténylegesen rögzített [notify forrás](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/src/NimBLECharacteristic.cpp#L272) változatlan; lokális eredmény nem hostvétel.

## Folytatási ellenőrzőpont — 4/8 / implementáció előtt

Elkészült: baseline és megőrzés ellenőrzése, időalapú debounce és hatgombos kiválasztási szabály rögzítése. Következő: firmware-ben és native trace-ekben egyaránt használt, fix méretű KeyboardState/reportbuilder; folyamatos GPIO frame-scan és scan utáni submit bekötése; elavult press/release számlálók kivezetése. Hátra: célzott trace-ek, mapping ellenőrzés, végső C3 build és512866/24420byte-hoz viszonyított méret, dokumentáció és lezáró megőrzési diff. Függőség/buildhook nem változik, így tiszta másolatos build most nem indokolt.

## Implementációs ellenőrzőpont — 4/8 / native trace-ek kész

Új `KeyboardConfig.h`, `KeyboardState.h` és `src/KeyboardState.cpp`: fix8×18 nyers/stabil állapot, cellánként uint32 utolsó nyers váltás, teljes kívánt report. `processScan` a teljes frame után20ms debounce-ot és a rögzített slotmegtartó kiválasztást futtatja; a BLE-send eredmény nem bemenete. A KeyboardReport egyenlőség minden8byte mezőjét figyeli. A régi `changeKeyboardUsage` és KeyboardHid::press/release kikerült az aktív API-ból; submitReport/lifecycle/security/LED út megmaradt.

`main.cpp` csak nyers GPIO-frame-et készít, majd feltétel nélkül feldolgozza, BLE nélkül is. A két20µs várakozás és a multiplexerek/GPIO-kód beállításai változatlanok; a1s offline ág nincs, a10ms együttműködő ciklusvárakozás megmarad. A teljes kívánt report változásakor egy submit történik a scan után; eredménye nem módosítja a KeyboardState-et. A későbbi ready/reconnect pótlás szándékosan nincs hozzáadva.

A diagnosztikai `key_api_ok/zero` mezők és keyResult megszűntek. Loopból aggregált `HB stable_edges` (összes elfogadott fizikai cellaváltás, nullacella/duplikátum is) és `report_changes` (teljes kívánt report változásai) váltják fel őket; mindkettő telítődő32bit számláló. A TX notify API/státusz és event queue jelentése megmarad; a ritka heartbeat és scan/loop max mérés továbbra is aktív.

**Célzott ellenőrzések:** `tools/check_state.cpp` hat rövid, konkrét elvárt reporttal megadott trace-csoportja és az átvezetett `tools/check_hid.cpp` mind144 cellás mapping ellenőrzése **PASS, exit0**. A firmware-ben használt KeyboardState.cpp közvetlenül natívan linkelve; g++15.2.0, C++11, -Wall/-Wextra/-Werror, ASan/UBSan. LeakSanitizer a korábban dokumentált sandbox ptrace korlát miatt detect_leaks=0; ASan/UBSan aktív. Nincs általános tesztkeret, tesztorákulumként másolt debounce/report algoritmus vagy BLE-host szimuláció. A LED forrása változatlan, annak sikeres3/8 ellenőrzését nem ismételjük indokolatlanul.

Következő: README/aktuális adatértelmezés frissítése és az egyetlen végső C3 build; utána méretdelta, megőrzési diff, tartós bin/ELF/source pár és lezárás. A függőségek és a buildhook változatlanok, tiszta másolatot nem építünk újra.5/8 nem indul.


## Ellenőrzések lezárása — 4/8

A tesztek közvetlenül a firmware-ben használt `src/KeyboardState.cpp` kódot fordítják, nem másolt algoritmust. Konkrét elvárt eredmények:

| Trace | Ellenőrzött eredmény |
| --- | --- |
| Pattogó press/release és függetlenség | A váltásai nem állítják újra B idejét: B130ms-nál már stabil, A csak140ms-nál. B160ms-től stabil felengedése179ms-nál még nem,180ms-nál elfogadott; A slotja megmarad. |
| uint32 időátfordulás | FFFFFFF8→0000000B =19ms, még nincs váltás; →0000000C =20ms, elfogadott press és release. A0 időbélyeg is érvényes. |
| Hat normál gomb és modifier | [04,05,06,07,08,09] mellett mindnyolc modifier lenyomása FF, felengedése00 bitmezőt ad; a hat slot változatlan. |
| Hetedik tartott gomb, slotmegtartás | Az első hat után a0A továbbra is fizikailag tartott. A04 felengedésekor0A belép a0. slotba új él nélkül. A04 új lenyomása nem szorítja ki; a05 felengedésekor04 az1. slotba lép. |
| Nulla és duplikátumok | Két04 cella és kétE0 cella egy04 + modifier01; az első példányok felengedése nem reportváltozás. A másodikak után üres report; a tartott nullacella csak fizikai állapot. |
| Teljes scan, változatlanság, BLE nélküli működés | Két release és egy press együtt csak a teljes végső[06] reportot eredményezi; azonos következő frame-nél reportChanged=false. A nyers/stabil/kívánt állapot BLE-objektum nélkül is frissül és megmarad. |
| Magyar kiosztás | Mind144 cella a befagyasztott2/8 referencia HID usage/modifier értékével egyezik. Reserved és minden üres slot0, külön bal/jobb modifier bitek. |

Ténylegesen futtatott parancsok, projektgyökérből, mind **exit0/PASS**:

```sh
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iinclude tools/check_state.cpp src/KeyboardState.cpp -o /tmp/keyboard-step4-20261007/check-state
ASAN_OPTIONS=detect_leaks=0 /tmp/keyboard-step4-20261007/check-state
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iinclude tools/check_hid.cpp src/KeyboardState.cpp -o /tmp/keyboard-step4-20261007/check-hid
ASAN_OPTIONS=detect_leaks=0 /tmp/keyboard-step4-20261007/check-hid
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step4-20261007/build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step3-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
```

Az egyetlen végső C3 build **SUCCESS, exit0,7,40s**. A már meglevő cache-ek használata mellett nem történt csomagletöltés vagy frissítés. PlatformIO Core6.2.0, espressif32@6.13.0, framework3.20017.241212+sha.dcc1105b = Arduino2.0.17 / IDF4.4.7, toolchain8.4.0+2021r2-patch5, esptool2.41100.0 (4.11.0), SCons4.41101.0. NimBLE2.5.1 teljes commit **5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb**, CD1.0.2 gitlink **aa6959c25a9ad7160fd515f73d8727aa86fc1ffd**. A deklarációk és a felhasznált könyvtárforrások változatlanok; a3/8 csomaglista a mentésben változatlan, történeti listaként megőrizve. Nincs új függőség, patch vagy buildhook. Emiatt a3/8-ban már sikeres tiszta másolatos buildet **nem ismételtük meg**. Nem futott új frissességi audit, Docker/CI vagy referenciaprojekt-build.

| Állapot | Flash használat (byte) | Statikus RAM (byte) |
| --- | ---: | ---: |
| Lezárt3/8 célbuild | 512866 | 24420 |
| Végső4/8 célbuild | **513294 /1027200 (50,0%)** | **25276 /327680 (7,7%)** |
| Változás | **+428** | **+856** |

A nagyobb statikus állapot cellánkénti időbélyegeket és külön nyers/stabil frame-et is tartalmaz. A PIO statikusRAM-adat nem mutatja a teljes runtime heap/stack fogyasztást. Hat, a3/8-nál is dokumentált upstream BLE_HS_HCI_ERR warning maradt (öt NimBLEClient, egy NimBLEServer); saját kódwarning vagy buildhiba nincs. HB/TX maximális számmezőkkel végzett szöveghossz-ellenőrzése310/238byte, a meglévő384byte formatter alatt; a soros kimenet továbbra is kihagyható és számlált, nincs várakozó kiírás.

## Az állapot és időzítés pontos korlátai

A20ms a **mintákban megfigyelt** változatlanság ideje. A minták közötti elektromos pattogás/rövid impulzus ebből nem bizonyítható. Minden teljes frame egy közös scan-végi millis időt kap; a GPIO-k egymás után olvasott fizikai mintái nem azonos pillanatú hardver-snapshotok. Az unsigned időeltérés a folyamatosan futó scanner normál uint32 átfordulását kezeli; teljes számlálóperiódusnyi megakadás után nem rekonstruál eltelt időt. A hatgombos választás a jelen stabil állapot és az előző **kívánt** report alapján determinisztikus, a notify eredménye nem befolyásolja a prioritást.

A ciklus a mintavétel, feldolgozás és esetleges egyetlen submit után delay(10)-et végez. Ez nem garantált10ms scan-periódus: már a két változatlan20µs beállási hely összesen3040µs/scan, ezen felül GPIO/API/feldolgozás és diagnosztika ideje adódik hozzá. A scan_us a mintavétel+debounce+report+esetleges submit, a loop_us a10ms várakozás és diagnosztika idejét is tartalmazza. A ritka BOOT/HB és a szinkronizált,16 elemű BLE-eseménysor megmaradt; callback nem fér hozzá a fizikai állapothoz, nem ír soros portot.

Sikertelen, NotReady vagy Disconnected submit után a KeyboardState **megőrzi** a fizikai és legutóbbi kívánt teljes reportot; ugyanazt változatlan scan után most nem küldi újra. Újabb stabil változás a legfrissebb teljes kívánt állapotra vált; régi offline élek nem gyűlnek visszajátszáshoz. Ez még **nem** kézbesítési/reconnect megoldás: egy kimaradt release vagy readiness előtti report pótlása nincs meg, ahogy a bontáson át tartott gomb tiltása sem. A helyi notify bool/status továbbra sem hostoldali ACK. A natív trace ezt az állapotfeldolgozást ellenőrzi; a tényleges BLE-hibát/kapcsolatváltást nem szimulálja.

Hardveren továbbra is ellenőrizetlen: a20ms küszöb megfelelősége és tényleges scan-idők, debounce rövid gombnyomásokra gyakorolt hatása, ghosting/táp/GPIO/mux/RF, diagnosztikai runtime események, pairing és Windows/Linux/Android HID-vétel, tartós BLE-stabilitás. Nincs szoftveres ghosting-szűrés vagy rövid lenyomások garantált vétele. A pinout,8×18 magyar mátrix,20µs beállási idők, descriptor, security, board/flash/partíció/TX-power és watchdog/brownout/reset policy változatlan. A teszthardver eredeti firmware-éhez és NVS-éhez nem nyúltunk.

## Tartós megőrzés és az5/8 pontos kiindulópontja

A4/8 bin/ELF/build-info/buildnapló/source archive/manifest/ellenőrzési összesítő/SHA256SUMS együtt: **`.local-snapshots/step-4-build-20261007/`**, a már meglévő helyi Git-kizárás alatt. A2/8 és3/8 korábbi mentései külön, változatlanok. Build ID **54adbb04939d-dirty-src625c1387b44f**, UTC **2026-10-07T10:19:27Z**. Bin fájlméret538640byte; SHA256 **cf9fad277ec147b0ee8620cba4aca9fca01ae02e3f0bfcfe70fd9f2e91f14d16**. ELF SHA256 **bf36e82ce7f05f4c8c3a3e2ed2d81bf4e85735d502151d0abcbd92ca42f96e28**. A bin fájlméret különbözik a PIO flash használatától. Későbbi hardveres próba az adott bin/ELF/JSON párhoz a meglevő115200baud naplómentéssel; ezt most nem hajtottuk végre.

**Elkészült4/8:** BLE-től független per-key szimmetrikus timestamp debounce, külön nyers/stabil állapot, teljes determinisztikus hatgombos report és független modifier bitek, folyamatos offline scan, legfeljebb egy változott report/scan, pontosított korlátos diagnosztika, célzott trace-ek és144-cellás mapping PASS, sikeres C3 build és méretösszehasonlítás. A LED és BLE callback/submit út változatlan; a korábbi sikeres LED-ellenőrzés érvényben marad. **Ebben a lépésben nincs hátralévő implementáció.**

**Következő feladatok kizárólag új5/8 kérésre:** a link/readiness/generation átadása és handle reuse kezelése; a kívánt report elválasztása a küldési állapottól és a sikertelen/NotReady submit korlátos retry/backpressure kezelése; kapcsolatváltási resync és a bontáson át tartott gomb csak felengedés+új lenyomás utáni engedélyezése. Ehhez a loop saját `keyboardState.raw()`, `.stable()` és `.desiredReport()` hozzáférései rendelkezésre állnak. A fizikai feldolgozást ezek a szabályok később sem állíthatják le. A tényleges hostoldali vétel és a hardveres debounce hangolása külön próbák feladata.

**Változáslista ebben a4/8 lépésben:** új `include/KeyboardConfig.h`, `include/KeyboardState.h`, `src/KeyboardState.cpp`, `tools/check_state.cpp`. Módosult `src/main.cpp`, `include/KeyboardReport.h`, `include/KeyboardHid.h`, `src/KeyboardHid.cpp`, `include/Diagnostics.h`, `src/Diagnostics.cpp`, `tools/check_hid.cpp`, `README.md`, `PROGRESS.md`. A többi1/8–3/8 módosítás és új fájl megőrizve, új staging/commit nélkül. Nincs dependency- vagy hookváltozás.

**Folytatási ellenőrzőpont —4/8 kész:** új sessionben először a dokumentum eleji aktuális állapotot és ezt a lezárt4/8 szakaszt, majd a teljes staged/unstaged/untracked és submodule állapotot olvasd. A lezárt forrás és a hozzá tartozó artifact a tartós step-4-build mentésben van; ne a régi HEAD-et tekintsd a jelen implementációnak. A korábbi dokumentumszakaszok történeti állítások, így a3/8 végi „4/8 nem kezdődött el” már nem aktuális. **Itt megállunk;5/8 nem kezdődött el.**


**Utolsó lezáró megőrzési ellenőrzés:** git diff --check és git diff --cached --check exit0; a Git-index byteonként változatlan a4/8 elejéhez képest. Az AUDIT SHA256854dac7b… és a teljes korábbi PROGRESS szöveg az aktuális első sor kivételével változatlan. A korábbi2/8–3/8 mentések mind28 fájlja SHA256 szerint megőrizve; CD és mindhárom referenciarepo tiszta. Csak a fenti9 meglévő fájl és4 új feldolgozó/trace fájl változott ebben a lépésben. A forrásfingerprint read-only ellenőrzése625c1387b44f, egyezik a megőrzött builddel; a dokumentáció lezárása nem módosít firmware-forrást. GPIO/mux/pinSetup/setup, KeyboardLayout, LED, BLE lifecycle/security/submit és buildkonfiguráció a3/8-hoz képest megőrizve; a BLE adapterből csak a már nem használt press/release API került ki. Nincs új build/test/feltöltés a sikeres ellenőrzések után, nincs5/8 implementáció.


# Felújítás — 5/8: munkamenet, elnyomás és korlátos küldés

## Kiinduló ellenőrzőpont

AGENTS.md nincs a célprojektben/workspace-ben vagy a releváns ősútvonalakon. AUDIT, lezárt4/8 PROGRESS, teljes staged/unstaged/untracked és CD submodule áttekintve. Az aktuális41 forrásfájl egyezik a tartós4/8 source archive manifestjével; a két staged gitlink-törlés és az összes korábbi módosítás megőrizve. Mindhárom referenciarepo tiszta. A korábbi helyi mentések minden fájljának hash-e, a Git-index és teljes diff a /tmp/keyboard-step5-20261007 bemeneti ellenőrzési mappában feljegyezve. A tartós4/8 mentés változatlan; nem a HEAD-ből rekonstruáljuk a baseline-t.

## Választott szabályok — implementáció előtt

- Callbackek kizárólag rövid kritikus szakaszban védett linkadatot módosítanak. Külön GAP connected, encryption (változatlan küldési security-feltétel), authenticated/bonded megfigyelés és keyboard notify subscription; generáció minden connect/disconnect és ready él esetén változik, ezért azonos handle és két scan közötti bontás/resubscribe is észrevehető.
- A loop által birtokolt munkamenet új generációnál eldobja a régi függő reportokat/elfogadott állapotot. Kezdetkor vagy nem-ready állapotban minden raw vagy stable szerint tartott fizikai cellát elnyom; csak legalább20ms-nyi megfigyelt stabil LOW oldja fel, akkor is, ha a pending press még nem vált stable HIGH-ra. Boot/első kapcsolat és resubscribe is konzervatívan felengedés+új press szabályú. Fizikai scan/debounce végig fut.
- Ready után külön, kötelező üres report barrier; helyi elutasításkor ez marad függő. A ready szakaszban keletkező szűrt teljes reportok kis,8 elemű FIFO-ba kerülnek, így torlódáskor egy rövid stabil press→release nem olvad automatikusan egyetlen üres végállapotba. Offline/pre-ready reportot nem queue-zunk.
- A FIFO túlcsordulásakor az elavult backlog eldobódik, üres helyreállítási barrier és legfrissebb szűrt állapot kerül a sorba. Ez ritka túlterheléskor köztes nyomásvesztést okozhat; számoljuk az overflow/drop értékeket, de a végállapot korrekcióját megőrizzük.
- Legfeljebb egy lokális submit/loop, legalább20ms próbálkozási térköz; LocalRejected után20→40→80→160→200ms-ra korlátozott növekedés, siker után20ms. Nincs végleges próbálkozási limit: egy release korrekciója azonos ready munkamenetben továbbra is újrapróbálható. NotReady/Disconnected nem notify-spin, session/generation check mindkét oldalon; a pontos retry-kezdő lépést a kész kód és trace rögzíti.

Elsődleges API: [ServerCallbacks](https://h2zero.github.io/NimBLE-Arduino/class_nim_b_l_e_server_callbacks.html), [Characteristic](https://h2zero.github.io/NimBLE-Arduino/class_nim_b_l_e_characteristic.html). A döntést a rögzített [NimBLE2.5.1 forrás](https://github.com/h2zero/NimBLE-Arduino/tree/2.5.1) alapján ellenőrizzük, nem masterre migrálunk. Következő: firmware-ben közvetlenül használt tiszta link/munkamenet-kód, adapter pre/post generációellenőrzés, korlátos diagnosztika és vezérelt natív trace-ek.


## Implementációs ellenőrzőpont —5/8 / vezérelt trace-ek kész

Új `KeyboardLink.h`: callbackben portMUX-szal védett64bit generáció és scalar linkadat, ugyanazon tiszta state-kód natívan ellenőrizve. Új `KeyboardSession.h/.cpp`: loop-owned elnyomási mátrix, hostnak szánt report,8 elemű FIFO, külön kötelező initial-empty állapot és token/generáció szerint ellenőrzött helyi elfogadás. A fizikai KeyboardState változatlan debounce-hoz kis `releasedForDebounce()` hozzáférést ad: rawLOW+stableLOW és20ms eltelt idő; a pending press korai LOW pattogása sem oldja fel az elnyomást.

A paraméterek a KeyboardConfig.h-ban: FIFO8, minimális send20ms, első reject utáni retry20ms, sorozatos rejectnél40/80/160/200ms cap; helyi siker után20ms. Nincs attempts-limit. Legfeljebb egy request/loop; a lastAttempt munkamenetváltáskor sem törlődik. Offline queue nincs. Ready alatt FIFO őrzi a stabil press/release reportokat; overflow8 régi reportot eldob, üres barrier és legfrissebb szűrt report marad. Az üres report localfalse esetén nem teljesült a sync.

A KeyboardHid submit generációt kap; notify előtti és utáni check, majd a loop completion még egyszer ellenőrzi a sessiont. A késői TX callback kizárólag attribúció nélküli diagnosztika. `prepareSession()` a loop új generáció észlelésekor helyileg üríti az olvasható GATT-value-t is; ez nem notify-siker vagy host release. GAP read/notify és handle-only stack API miatt teljes versenymentességet nem állítunk.

A9 célzott native trace-csoport **PASS,exit0**, g++ C++11/Wall/Wextra/Werror/ASan/UBSan (a korábbi sandboxkorlát miatt LeakSanitizer kikapcsolva). Boot/held normál+modifier, pending raw debounce és LOW pattogás, duplikált fizikai cellák, offline/late-ready, két loop közötti same-handle reconnect/ready off-on, hibás üres-sync, rövid press→release kezdeti torlódás alatt, tartós release-retry, overflow-helyreállítás, régi token/generáció, NotReady/Disconnected és uint32 retry-wrap konkrét elvárt reportokkal ellenőrizve. Firmware-kód közvetlenül linkelve; nincs általános framework/host ACK.

Következő: érintett4/8 regressziók, végső egyetlen C3 build és513294/25276byte-hoz viszonyított méret; lezáró forrás/bin/ELF mentés és megőrzési ellenőrzés. A deps/buildhook változatlan, tiszta másolat új buildje nem indokolt.6/8 nem indul.


## Lezárt munkamenet- és küldési szabályok —5/8

A források saját projektbeli kódok, nincs új külső könyvtár, átvett fork vagy upstream patch. A4/8 fizikai debounce/reportbuilder megmarad: a `KeyboardState::desiredReport()` innentől **szűretlen fizikai report**, a `KeyboardSession::wantedReport()` a hostnak szánt szűrt állapot. A FIFO az aktuális ready munkamenet függő reportjait, az `accepted_`/localValid pedig az ugyanott közvetlen API-val lokálisan elfogadott utolsó reportot tartja. Ezek nem hostoldali ACK-adatok.

| Szabály | Kész implementáció és pontos jelentés |
| --- | --- |
| Link snapshot | `include/KeyboardLink.h:6`, `src/KeyboardHid.cpp:78`: GAP connected, handle, encrypted, authenticated, bonded, keyboard notify subscription és64bit generáció; másolat egy rövid portMUX alatt. Ready=connected AND encrypted AND subscribed. Auth/bond információ, nem új security-követelmény; a bonding/MITM/SC kérés és IO-policy változatlan. |
| Új munkamenet | `KeyboardLink.h:18`: minden valódi connect és aktuális peer disconnect növeli a generációt; security/subscription módosítás csak akkor, ha a ready értéke vált. A ready false→true új aktív küldési munkamenet; false szakasz nincs küldés. Azonos-handle connect/disconnect és ugyanazon handle ready off/on két scan között is más generációt ad. Ismételt azonos subscription vagy más peer callbackje nem módosítja a sessiont. A64bit körbefordulása elméleti korlát, gyakorlati boot-élettartamra nem támaszkodunk handle-egyezésre. |
| Cellánkénti elnyomás | `src/KeyboardSession.cpp:25`: boundary vagy nem-ready során raw OR stable szerint tartott **fizikai cella** maszkolva. Csak rawLOW AND stableLOW és utolsó nyers váltás óta legalább20ms után oldódik. `KeyboardState.cpp:4` ugyanazt a fizikai időbélyeget használja, nincs másik hangolt debounce. Ez pending pressnél is megköveteli a stabil LOW-ot. A nullacella is fizikailag követett, de reportban kimarad; duplikált usage másik cellája önállóan használható. |
| Boot/első host/resubscribe | Ugyanaz a konzervatív felengedés+új press szabály: már raw vagy stable HIGH cella nem kerül az új hostreportba. A kész snapshot előtti/első scan alatti új press időpontját nem tudjuk külön bizonyítani, ezért az is elnyomódhat. A tényleges nyers/stabil fizikai állapot nem törlődik, scan/debounce offline is fut. |
| Kötelező üres sync | Új ready generáció clear queue/localValid után initialEmpty=true. Az üres notify **LocalAccepted** eredményéig külön függő, nem küldünk normál FIFO-headet. Az ezután küldött FIFO csak a **ready szakaszban** megfigyelt friss szűrt állapotváltozásokat tartalmazhatja, időrendben; ezek között megőrzi az üres sync torlódása alatt keletkezett rövid press/release-t is. A legutolsó elem a jelenlegi szűrt állapot. Offline vagy pre-ready report nincs benne. |
| Sor és overflow | `KeyboardSession.cpp:8`: legfeljebb8 teljes8byte report; azonos szomszédos report nincs duplázva. Ha betelik, a8 backlogelem eldobva, üres helyreállítási barrier és szükség esetén az aktuális report kerül be. A külön initialEmpty ettől nem lesz sikeres. A meglévő időkapu megmarad; overflow nem indít burstöt. Köztes press/release veszhet el, a végállapot korrekciója megmarad. A helyreállítás egy tovább tartott, engedélyezett gombnak új hostélt adhat; ez dokumentált túlterhelési kompromisszum, nem NKRO vagy delivery-garancia. |
| Próbálkozási térköz | `include/KeyboardConfig.h:9`: alap20ms, első LocalRejected után20ms, ismételt reject után40/80/160/200ms cap; helyi siker után20ms. Egy loopban egy submit, nincs belső while/retry, task vagy extra delay. Az unsigned32 elapsed kezeli az átfordulást; a gate sessionváltáskor is megőrzi az előző próbálkozás idejét, így maximum50 alkalmazási attempt/s, a scan-idő miatt a valós gyakoriság ennél kisebb is lehet. Ezek kezdeti policyértékek, nem mért BLE connection intervalok. |
| Release és hibák | LocalRejected nem popolja a headet. Nincs végleges attempts-limit vagy timeout miatti végállapot-eldobás: azonos ready sessionben a release/üres korrekció függő marad és újrapróbálható. A FIFO sorrendje miatt korábbi függő report késleltetheti a release-t; tartós API-hiba alatt kézbesítés nem garantálható. NotReady/Disconnected/StaleSession esetben a completion lezárja az adott attemptet, de következő teljes frame/link update előtt nem enged új attemptet; nem-ready snapshot alatt nincs request/notify. Új generáció a régi queue-t tudatosan eldobja. |
| Helyi eredmény | `KeyboardSession.cpp:76`: request token+generation és post-send snapshot egyeztetés; régi eredmény sem a queue-headet, sem a localValid/accepted állapotot nem módosítja. A firmware completion **szinkron** notify bool után történik; egy időben egy request. A callback nem ezt a completiont használja. |

A headFailed/retries számláló a legutóbbi helyi elutasítás után ugyanazon függő küldési lánc további attemptjeit számolja; queue-kompaktálás/új session után ez a lánc újraindul. A retry időkapu overflowkor nem törlődik. A20ms fizikai debounce-küszöb és a slotmegtartó6-key builder nem változott.

## Advertising és pontos API-korlátok

A rögzített NimBLEServer.cpp **507–510** bontáskezelője az onDisconnect után `startAdvertising()`-ot hív, ha m_advertiseOnDisconnect true. A projekt `begin()` már3/8 óta `advertiseOnDisconnect(true)`-t állít; változatlan maradt, nincs indokolt további restart. Forrás: [2.5.1 szerverforrás](https://raw.githubusercontent.com/h2zero/NimBLE-Arduino/2.5.1/src/NimBLEServer.cpp), [hivatalos advertiseOnDisconnect API](https://h2zero.github.io/NimBLE-Arduino/class_nim_b_l_e_server.html). A commitra rögzített helyi upstream HEAD ellenőrizve:5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb. A web a teljes-commit szerverlinkre cache-miss hibát adott; a tagforrás és API elérhető, a pontos commit forrását helyileg is olvastuk. Restart sikerét a belső hívásból nincs külön alkalmazási return, hardveren nem ellenőriztük. Nincs begin/deinit/reboot, security-gyengítés vagy bondtörlés hozzáadva.

A `submitReport(report, expectedGeneration)` pre-check, GATT-value beállítás utáni check és post-notify snapshot alapján StaleSessiont is visszaadhat; a controller a legutolsó snapshotot még egyszer ellenőrzi. Kritikus szakasz csak scalar linket véd, nem fog át GATT/notify/Serial hívást. **Fennmaradó verseny:** ha az utolsó pre-check után a stack bont és új peernek ugyanazt a handle-t osztja ki, a handle-only notify a régi byte-okat elérhetővé teheti az új peernek, mielőtt a post-check ezt elavultnak jelöli. A post-check megakadályozza a helyi jóváírást, a már beadott stackműveletet nem vonja vissza. Ez az API-val teljesen nem atomizálható; nem állítunk minden rádiós interleavingre bizonyított elnyomást.

A helyi olvasható GATT reportérték új generáció első loop-észlelésekor ürül (`KeyboardHid.cpp:85`), readiness előtt is. Ez csökkenti a régi report megmaradását, de a callback és a következő scan/loop közötti host-read versenyablakot nem zárja ki; a setValue is külön stackművelet. Ez **nem** a kötelező üres notification teljesülése, és nem egy callbackből módosított mátrix/report.

A rögzített szerver NOTIFY_TX ágában **556–578** a ConnInfo nincs feltöltve és nincs saját request-token/generáció. Az onStatus kizárólag globális helyi TX-diagnosztika, sem queue-popot, sem aktuális report-retryt nem okoz. Egy késői státusz így nem fejez be új sessiont. Ugyanakkor egy bool=true után később jelentett aszinkron hibából nem tudunk biztosan azonos reportkorrekciót képezni. Forrás: [rögzített szerver](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/src/NimBLEServer.cpp#L556), [explicit peer notify forrás](https://github.com/h2zero/NimBLE-Arduino/blob/5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb/src/NimBLECharacteristic.cpp#L272), [Characteristic API](https://h2zero.github.io/NimBLE-Arduino/class_nim_b_l_e_characteristic.html). Nincs HID-host ACK-protokoll. Megszakadt kapcsolatban a régi host felengedése nem garantálható; a helyi siker nem host HID-feldolgozás.

## Diagnosztika az5/8 állapotban

BOOT/10s HB/TX, scan/loop és maximum, heap/minimum,16 elemű callback queue és számlált soros kihagyás megmaradt. Plusz legfeljebb egy **SESSION** sor/heartbeat. Nincs reportbyte/billentyűkód/cella koordináta naplózás, callbackben nincs hosszú soros kiírás. A loop snapshot és scalar számlálói nem callback-adatok.

| Mező | Jelentés |
| --- | --- |
| HB stable_edges / physical_report_changes | Elfogadott fizikai cellaváltás / a szűretlen fizikai report változása. A korábbi report_changes átnevezve, mert immár külön szűrt report is van. |
| SESSION gen/gap/enc/auth/bond/sub/ready | A loop által legutóbb alkalmazott link-snapshot; nem egy későbbi callback állapotának szimultán ígérete. |
| suppressed / pending / sync | Elnyomott fizikai cellaszám / FIFO elemszám0–8 / külön függő initial-empty. A sync report nem része a pending elemszámnak. |
| local_valid / wanted_changes | Van ugyanebben a sessionben lokálisan elfogadott teljes report / szűrt reportváltozások boot óta. Nem host ACK. |
| attempts / retries / rejected | Controller által kiadott submitok / ugyanazon hibás küldési lánc újrapróbálásai / LocalRejected eredmények. A TX notify_attempts csak a tényleges stack notify hívás, ezért eltérhet. |
| overflow / discarded / stale | FIFO-kompaktálások / kifejezetten overflow miatt eldobott backlogreportok / régi token/generáció vagy not-ready completion elvetése. A sessionváltáskori queue-eldobás nincs discardedként számolva. |
| TX api_accepted/rejected / status_ok/error | Közvetlen notify bool / attribúció nélküli aszinkron státuszösszesítés. Egyik sem garantált hostvétel, és az utóbbi nincs a jelenlegi sessionhöz rendelve. |

A számlálók telítődők; a session/readiness/tokens64bit generációja nem számlálódiagnosztika. HB/TX/SESSION konzervatív számmező-szélességekkel324/239/262byte, a384byte formatter alatt. A három sor az1024byte TX-ring elvi kapacitása alatt marad, de a tényleges UART-szabadhely továbbra is dönt: egy sor kihagyható és log_dropként számolt, nincs új blokkoló várakozás.

## Ellenőrzések és végső build —5/8

`tools/check_session.cpp` közvetlenül a firmware KeyboardState/KeyboardLinkState/KeyboardSession kódját használja, vezérelt **lokális** küldési eredménnyel. Kilenc rövid csoport konkrét numerikus reportokat és időpontokat ellenőriz; nem másolt implementáció az orákulum:

- Bootkor és GAP/readiness előtt tartott04+E0: üres sync után is elnyomott; LOW pattogás után csak90ms stabil release oldja fel, majd120ms új press reportja modifier01/key04. Bontás/reconnect után ugyanez megismételhető.
- Pending raw press, mely még nem ért stableHIGH-ra:35ms utolsó LOW után54ms-nál még suppressed,55ms-nál felszabadult. Másik fizikai04 cella új press-e önállóan működik, az elnyomott első04 tovább tartása nem jelenik meg a reportban.
- Offline teljes press/release és GAP alatti, titkosítás/feliratkozás előtti press/release: ready után kizárólag üres sync, nincs fantomreport.
- Ugyanazon7 handle disconnect+connect és unsubscribe+resubscribe két loop között: eltérő generation, régi queued/accepted állapot eldobva, új üres sync. Encryption-loss/recovery és rossz handle callbackjei külön ellenőrizve.
- Initial-empty reject0/20ms,59ms-nál még nincs attempt,60ms-nál helyi elfogadás. Közben readyben stabil press/release a FIFO-ban megmarad:80ms key04,100ms üres, változatlan120ms-nál nincs küldés.
- Press reject50/70ms; release95ms-ra stabil, még pending a press mögött.110ms press-siker után a release130/150/190/270/430/630ms reject mellett is megmarad;830ms local-success végállapot üres. Nincs végleges retry-limit.
- Nyolc report backlog után a kilencedik változás overflowt és discarded8-at ad; következő küldés üres barrier, amely első reject után is pending marad, majd04 és végső üres report. Nincs korábbi offline/session report.
- Régi generation és régi request-token helyi sikernek jelölt eredménye nem teljesíti az új in-flight üres reportot; NotReady/Disconnected nem indít completionből spin-t.
- Retry timer FFFFFFF8→0000000B még19ms/no attempt, →0000000C20ms/attempt; későbbi press/release rendben. A fizikai timestamp-wrap a megismételt4/8 regresszióban is PASS.

Parancsok projektgyökérből, **ténylegesen végrehajtva, mind exit0**:

```sh
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iinclude tools/check_session.cpp src/KeyboardSession.cpp src/KeyboardState.cpp -o /tmp/keyboard-step5-20261007/check-session
ASAN_OPTIONS=detect_leaks=0 /tmp/keyboard-step5-20261007/check-session
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iinclude tools/check_state.cpp src/KeyboardState.cpp -o /tmp/keyboard-step5-20261007/check-state
ASAN_OPTIONS=detect_leaks=0 /tmp/keyboard-step5-20261007/check-state
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iinclude tools/check_hid.cpp src/KeyboardState.cpp -o /tmp/keyboard-step5-20261007/check-hid
ASAN_OPTIONS=detect_leaks=0 /tmp/keyboard-step5-20261007/check-hid
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step5-20261007/build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step3-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
```

A session trace, az érintett4/8 state regressziók és mind144 magyar HID mapping **PASS**. G++15.2.0, C++11, Wall/Wextra/Werror, ASan/UBSan; a LeakSanitizer a már dokumentált sandbox ptrace korlát miatt kikapcsolva. LED-helper/output feldolgozás változatlan, annak korábbi sikeres ellenőrzését indokolatlanul nem ismételtük. A natív helyettesítő nem a BLE-adapter/rádió/host integráció emulátora; valódi stack callback-interleaving és host-read verseny nincs igazolva.

Az **egyetlen végső C3 build SUCCESS,exit0,9,60s**. Nincs csomagletöltés vagy dependency-frissítés. Core6.2.0, espressif32@6.13.0, framework3.20017.241212+sha.dcc1105b = Arduino2.0.17/IDF4.4.7; RISC-V8.4.0+2021r2-patch5, esptool2.41100.0 (4.11.0), SCons4.41101.0. NimBLE2.5.1 commit5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb, CD1.0.2 gitlinkaa6959c25a9ad7160fd515f73d8727aa86fc1ffd; a build gráfja ugyanaz. A korábbi csomaglista történeti példányként a mentésben megőrizve. A buildhook/dependency/config byteazonos a4/8 baseline-nal, ezért a tiszta másolatos build **nem ismétlődött**. Unit-framework, CI/Docker, teljes frissebb-verzió audit és referencia-build nem futott.

| Állapot | Flash használat (byte) | Statikus RAM (byte) |
| --- | ---: | ---: |
| Lezárt4/8 | 513294 | 25276 |
| Végső5/8 | **516564 /1027200 (50,3%)** | **25684 /327680 (7,8%)** |
| Delta | **+3270** | **+408** |

A PIO RAM-mutató statikus adat, nem a teljes runtime heap/task stack terhelés. A korábbi hat NimBLE BLE_HS_HCI_ERR warning maradt, nincs saját kódwarning/buildhiba. A sikeres build és trace nem pairing-, hostvétel-, beragadásmentesség- vagy tartós stabilitásbizonyíték.

## Tartós artifact, korlátok és folytatási ellenőrzőpont —5/8 kész

Mentés: **`.local-snapshots/step-5-build-20261007/`**, a meglévő helyi Git-kizárás alatt. Végső forrásarchive+manifest, firmware.bin/ELF/build-info/buildlog, baseline index/diff/manifest, checks és SHA256SUMS együtt. Build ID **54adbb04939d-dirty-srcf3c6dd6e878e**, UTC **2026-10-07T12:25:15Z**. Bin fájlméret542432byte, SHA256 **61a76fdd52f03d8e5315a77b59c94a52d336801cb20a09adbef760bb682b922b**; ELF SHA256 **3464ed57204e9e51a302aa30ac2435b01c399ddc2c9bcf9061c8fddf923cbdd3**. A bin fájlméret nem azonos a PIO flashhasználattal. A2/8–4/8 korábbi mentései külön és változatlanok.

**Változáslista5/8:** új `include/KeyboardLink.h`, `include/KeyboardSession.h`, `src/KeyboardSession.cpp`, `tools/check_session.cpp`. Módosult `include/KeyboardConfig.h`, `include/KeyboardState.h`, `src/KeyboardState.cpp` (csak release-idő hozzáférés), `include/KeyboardHid.h`, `src/KeyboardHid.cpp`, `src/main.cpp` (loop bekötés), `include/Diagnostics.h`, `src/Diagnostics.cpp`, `README.md`, `PROGRESS.md`. Minden korábbi módosítás megőrizve, új staging nélkül; a két már stage-elt gitlinktörléshez nem nyúltunk. GPIO/mux/20µs mintavétel/pinSetup/setup, magyar layout, hatgombos builder,20ms debounce, descriptor/security/advertising, dependency/config/hook és LED-output változatlan. Nincs OTA/új funkció, flash/partíció/TX-power/watchdog/brownout/reset-policy módosítás.

**Elkészült:** szinkronizált link/readiness/generáció, cellánkénti stabil-release elnyomás, initial-empty barrier, szétválasztott fizikai/wanted/pending/local-accepted állapot, korlátos FIFO és időzített retry, stale-result elvetés, ritka értelmezhető diagnosztika; célzott trace/regressziók és végső C3 build sikeresek. **5/8 implementációban nincs hátralévő munka.**

Hardveren később a meglévő115200 capture és pontos bin/ELF/JSON pár mellett: boot-held és first-pairing, late auth/subscription, tartott normál/modifier melletti ismételt reconnect, host sleep/wake, gyors stabil press/release, túlterhelés/notify-error és hosszabb használat. Windows/Linux/Android külön hostpróba; a régi NVS/bond és valódi module/flash/pin+táp/RF kérdéseket a korábbi terv szerint ellenőrizni kell. A20ms küszöb, scan/periódus és ghosting hardveresen továbbra sem igazolt. Most nincs portnyitás/feltöltés/flash vagy NVS-törlés, a régi tesztfirmware megmaradt.

**Pontos folytatás:** új sessionben a dokumentum eleji aktuális állapotot és ezt a lezárt5/8 részt olvasd, majd teljes staged/unstaged/untracked/submodule ellenőrzés. A végleges forrás és artifact a tartós step-5-build mentés, nem a régi HEAD vagy egy régi .pio könyvtár. A korábbi lezárások történeti eredmények. A6/8 tartalmát külön következő feladat határozza meg; most nem indul, nem végzünk további javítást, stackfrissítést vagy hardverpróbát. Nincs commit/push/publikálás.


**Utolsó lezáró megőrzési ellenőrzés:** a Git-index byteonként változatlan, git diff --check és git diff --cached --check exit0. Mind43 korábbi helyi mentett fájl SHA256-ja egyező; a történeti PROGRESS teljes szövege az első aktuális sor kivételével megmaradt, AUDIT854dac7b… változatlan. CD gitlink tiszta és ugyanaz a commit, mindhárom referenciarepo tiszta. Csak a fenti10 korábbi fájl és4 új session/link/trace fájl változott. GPIO mintavétel/pinSetup/setup, BLE descriptor/begin/security/advertising és LED-output byteonként összevetve a lezárt4/8 forrással, egyező. A forrásfingerprintf3c6dd6e878e egyezik a builddel; nincs indokolatlan újrafordítás vagy tesztismétlés. A végső forrásarchive és manifest byteonként ellenőrizve, artifactokhoz SHA256SUMS készült. **5/8 kész; itt megállunk,6/8 nem kezdődött el.**


# Felújítás —6/8: BLE-only konfiguráció és hardverpróba-bemenetek

**Kiinduló ellenőrzőpont:** AGENTS.md nincs a célprojektben/workspace-ben/releváns ősökben. AUDIT és lezárt5/8, teljes staged/unstaged/untracked/submodule állapot áttekintve. A45 aktuális forrásfájl egyezik az5/8 source-manifesttel, a referenciarepók és CD tiszták. Index/diff/manifest, korábbi mentések hash-e /tmp/keyboard-step6-20261007 alatt feljegyezve; a tartós5/8 forrás/bin/ELF mentés változatlan. A két staged gitlinktörléshez nem nyúlunk.

**Igazolt célzott módosítások:** a régi std=c++14/O2 deklarációkat a baseline framework effektív gnu++11/Os beállítása felülírja; eltávolításuk a valós standard/optimalizálás megőrzése, nem új policy. A nem olvasott ESPFC_DEBUG_SERIAL/ESP32C3/BLE makrók kikerülnek, egyhostos NimBLE konfiguráció megmarad. A géphez kötött upload/monitor portok helyett PlatformIO-portválasztás és dokumentált CLI felülírás; napló115200, örökölt upload921600 változatlan. A régi HTTP uploader nem aktív hook, megmarad egyértelmű történeti/inaktív megjelöléssel.

Következő: egyszeri verbose C3 build az effektív flagekhez, meglévő partíció/artifactok statikus olvasása, pinned esptool4.11 syntax ellenőrzése kizárólag offline helpből, rövid README/7 előfeltételek és végső megőrzés. Runtime teszt nem szükséges, deps/hook nem változik, tiszta másolat nem indokolt.


## Lezárt konfiguráció és ellenőrzések —6/8

Módosult kizárólag `platformio.ini`, `platformio_upload.py` eleji történeti figyelmeztetés, `README.md`, `PROGRESS.md`. Runtime források és `KeyboardConfig.h` byteazonosak az5/8-cal: debounce20ms, FIFO8, minimum send20ms, retry20/40/80/160/200ms; egy helyen követett, nem bemért kezdőparaméterek. Az5/8 állapotkezelése és korlátai érvényben maradnak.

A `-std=c++14`/`-O2` deklarációk kikerültek: a rögzített framework `tools/platformio-build-esp32c3.py:52,58` gnu++11/Os flageket ad, az auditban is ezek voltak effektívek. A mostani tényleges main.cpp compilerparancsban **−std=gnu++11 és −Os** igazolt, nincs14/O2. A nem olvasott ESPFC_DEBUG_SERIAL/ESP32C3/BLE makrók, üres flag-interpoláció és inaktív ESP8266 közös opciók kivezetve. Az alkalmazás BLE-only, nincs módválasztó; az egyhostos CONFIG_BT_NIMBLE_MAX_CONNECTIONS=1 megmaradt. Egyetlen NimBLE commit és CD gitlink, nincs secret/WiFi/OTA/webes dependency. A platform/package pinek és az egyetlen aktív extra_script (`prepare_build.py`) változatlanok. A régi HTTP uploader sehol nincs hookként bekötve, csak fejlécben inaktív/történeti jelölést kapott; eredeti tartalma és a nem buildelt src/certs.h megőrizve.

Az upload_port/monitor_port és helyi /dev/ttyUSB1 értékek nincsenek az aktív konfigurációban. Uploadhoz később CLI `--upload-port ELLENORZOTT_PORT` vagy PLATFORMIO_UPLOAD_PORT, monitorhoz `--port ELLENORZOTT_PORT`; egyébként PlatformIO-portválasztás. A port a fejlesztői géphez tartozik, nem a BLE-host OS-éhez. Monitor/firmware115200, örökölt upload921600 változatlan és hardveren ellenőrizetlen. [PlatformIO konfiguráció](https://docs.platformio.org/en/latest/projectconf/index.html), [upload_port](https://docs.platformio.org/en/latest/projectconf/sections/env/options/upload/upload_port.html), [monitor_port](https://docs.platformio.org/en/latest/projectconf/sections/env/options/monitor/monitor_port.html).

### Builderedmény és korlát

A konfigurációváltozás utáni verbose build **FAILED,exit1,8,00s**: az alkalmazás lefordult/linkelt, de a SCons4.11.1 verbose CommandAction print útja a firmware.bin lépésnél `_Null + str` TypeErrorral leállt. Ezt nem tüntetjük sikeres buildnek. A compilerparancsból a tényleges flagek kinyerhetők voltak. A szokásos nem-verbose build, ugyanazokkal a változatlan csomagokkal **SUCCESS,exit0,3,12s**; firmware.bin és ELF létrejött. A sikertelen verbose ellenőrzést emiatt indokolt volt folytatni, a sikeres buildet nem ismételtük. Nincs dependency/SCons/framework patch vagy frissítés.

```sh
# Első, verbose ellenőrzés: a fenti SCons printer hibával FAILED.
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step6-20261007/build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step3-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3 -v
# Végső, sikeres normál build:
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step6-20261007/build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step3-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
```

Flash **516564 /1027200 byte**, statikus RAM **25684 /327680 byte**; az5/8-hoz képest **0 /0 byte**. Ugyanaz a Core6.2.0, espressif32@6.13.0, Arduino2.0.17/IDF4.4.7 framework3.20017.241212+sha.dcc1105b, toolchain8.4.0+2021r2-patch5, esptool2.41100.0/4.11.0, SCons4.41101.0. NimBLE2.5.1 commit5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb és CD aa6959c25a9ad7160fd515f73d8727aa86fc1ffd változatlan. Nincs csomagletöltés. A verbose teljes fordításban a korábbi hat upstream NimBLE warning maradt; nincs saját forráshiba. Buildbekötés/dependency nem változott, nincs tiszta másolat, runtime-teszt, CI/Docker/ref build vagy új audit.

## Igazolt buildadatok és még ismeretlen hardver

| Adat | Jelen buildben igazolt | Fizikailag ellenőrizendő |
| --- | --- | --- |
| Board/MCU/CPU | esp32-c3-devkitm-1 preset; esp32c3;160MHz | ESP-C3-12F konkrét változat/bekötés; preset nem DevKitM fizikai board igazolása |
| Flash | INI2MB, QIO/80MHz; qio_qspi SDK | Valódi kapacitás, chip/modulváltozat, QIO/80MHz üzemi helyesség |
| Image header | App és bootloader **2MB/80MHz/DIO**, esptool image_info offline olvasással | Header nem flashmérés. A pinned platform `_get_board_flash_mode()` QIO→DIO image-mappingje, nem mostani konfigurációcsere |
| Méretlimit | custom.csv app0 alapján1027200byte; az INI2097152 kezdeti maximum_size mezőjét a platform felülírja | Az eredeti firmware partíciója/aktív slot ismeretlen; nem a2MB feliratból következtetünk |

Forrás: a helyi pinned board JSON, framework/platform builder és kész artifactok; [platform6.13 builder](https://github.com/platformio/platform-espressif32/blob/v6.13.0/builder/main.py#L88), [Arduino2.0.17 builder](https://github.com/espressif/arduino-esp32/blob/2.0.17/tools/platformio-build.py#L209).

Az5/8 valódi partitions.bin statikus visszaolvasása a framework gen_esp32part.py-val **exit0**, parsing/verification sikeres. A6/8 partitions.bin byteazonos vele, a custom.csv sem változott:

| Partíció | Offset | Méret | Vége (kizáró) |
| --- | --- | --- | --- |
| nvs | 0x9000 | 0x4000 =16384 | 0xD000 |
| otadata | 0xD000 | 0x2000 =8192 | 0xF000 |
| phy_init | 0xF000 | 0x1000 =4096 | 0x10000 |
| app0 /ota_0 | 0x10000 | 0xFAC80 =1027200 | 0x10AC80 |
| app1 /ota_1 | 0x110000 | 0xD0000 =851968 | 0x1E0000 |

A két OTA-nevű slot történeti layout, nem aktív OTA-kód. Nem rendeztük át és nem töröltünk adatot. [IDF4.4.7 partíciók](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32c3/api-guides/partition-tables.html).

### Feltöltési artifactterv — csak provenance, nem indítandó upload

A framework FLASH_EXTRA_IMAGES és a platform ESP32_APP_OFFSET alapján, nem általános ESP32 példából:

| Image | Platform offset | Forrás a jelen buildben |
| --- | --- | --- |
| bootloader.bin | **0x0000** | Build/esp32c3; C3 ág, nem ESP32 0x1000 |
| partitions.bin | **0x8000** | Build/esp32c3, custom.csv alapján |
| boot_app0.bin | **0xE000** | Pinned framework tools/partitions,8192byte |
| firmware.bin | **0x10000** | Build/esp32c3, app0;542432byte, belefér1027200-ba |

Az ELF/build-info nem flashimage: hibakereséshez megőrzendő. A framework/platform helyi `platformio-build.py:209–215` és `builder/main.py:305,548` az offsetek forrása, upstream linkek fent. **Feltárt első-írási ellenőrzési pont:** boot_app0@E000 mérete2000h, tartomány[E000,10000); ez a saját otadata **második szektorát** és a teljes phy_init tartományt érintené. Nem fedi az egész[D000,F000) otadatát. A régi otadata első szektora/slotválasztása és az eredeti partíció ismeretlen. Ez bizonyított tartományeltérés, nem bizonyított jelenlegi boot/hardverhiba. Nem módosítjuk a táblát/offsetet, és nem javaslunk vak platform-uploadot: az első írás tervét a régi flashmentés partíció/otadata adata alapján kell külön összeállítani. NVS/bond megőrzéséről ott kell dönteni, nem automatikus erase-szel.

## A7/8 előtt szükséges minimum bemenetek

1. Ellenőrzött soros adapter és pontos port (pl. Linux stabil by-id vagy fejlesztői Windows COM), megfelelő logikai szint/közös GND, EN/bootmód hozzáférés és a használt modul dokumentált booteljárása. Most nem nevezünk meg kitalált fizikai bootbekötést.
2. Konkrét ESP-C3-12F flashváltozat/moduljelölés; majd ROM flash_id eredmény. A jelen2MB konfiguráció nem helyettesíti. A tényleges pin/mux/táp bekötés ellenőrzése: forrás szerinti sor6, oszlop7+18/19, címzők9/8/5/−1 és4/3/2/1, S3/E/táp/jelszint és speciális GPIO-k a korábbi audit alapján.
3. A kizárólag hardveren meglevő régi firmware **lehetőség szerinti teljes flashmentése első írás előtt**, méret/hash ellenőrzéssel, biztonságos privát helyen. A teljes dump firmware-t, boot/partíció/otadata/NVS-bond adatot is őrizhet, nem ELF vagy forrás; nem tartalmazza a host pairing-adatbázisát vagy az eFuse állapotot. Olvasási/védelmi korlát esetén dokumentálni kell, mi menthető és mi nem; ezután az első írásról külön döntés, nem automatikus továbbhaladás.
4. Négy külön állapot: régi firmware és flashlayout; eszköz NVS/bond; célhostok saját pairing-bejegyzései; új image/megfelelő ELF/build-ID. Nem állítjuk, hogy új firmware feltöltése automatikusan megtartja/kompatibilissé teszi a régi bondokat. Nincs host-bond vagy eszköz-NVS törlés előírva.
5. Új image/ELF/build-info/extra image-ek és offsetmanifest együtt, az alábbi6/8 mentésből. Az első írás előtt a boot_app0/otadata eltérés és a bootolt app kiválasztása tisztázandó. Windows/Linux/Android hostpróba külön, nem a fejlesztői port működésének következménye.

### Későbbi eszközparancsok — most csak dokumentáció

A syntax a tényleges tool-esptoolpy2.41100.0 esptool**4.11.0** verziója és offline `read_flash --help`, `flash_id --help`, illetve az argument-parser alapján ellenőrizve. A latest dokumentációt nem másoltuk vakon: itt **aláhúzásos**4.x parancsnevek szerepelnek. [Esptool háttér](https://docs.espressif.com/projects/esptool/en/latest/esp32c3/esptool/basic-commands.html), [4.11.0 parser](https://github.com/espressif/esptool/blob/v4.11.0/esptool/__init__.py). A v4 dokumentációs URL a webes eszközzel nem volt elérhető; a helyi pinned source/help volt az irányadó.

Kizárólag későbbi, hozzáférhető hardveren, ellenőrzött porttal és **kézzel elért ROM-download módban**:

```sh
# Az utak/port HELYKITÖLTŐK; a projektből nem fut automatikusan.
KBD_PYTHON=/ellenorzott/build-venv/bin/python
KBD_ESPTOOL=/ellenorzott/pio-core/packages/tool-esptoolpy/esptool.py
KBD_PORT=ELLENORZOTT_PORT
KBD_BACKUP=/privat/mentes/regi-teljes-flash.bin
"$KBD_PYTHON" "$KBD_ESPTOOL" --chip esp32c3 --port "$KBD_PORT" --baud 115200 --before no_reset --after no_reset --no-stub flash_id
"$KBD_PYTHON" "$KBD_ESPTOOL" --chip esp32c3 --port "$KBD_PORT" --baud 115200 --before no_reset --after no_reset --no-stub read_flash 0x0 ALL "$KBD_BACKUP"
sha256sum "$KBD_BACKUP"
```

ALL a4.11 parserben kis/nagybetű-független, és a felismert flashkapacitás végéig olvas, nem a projekt2MB-jáig. Ha nem detektálható vagy védett a flash, az olvasás nem sikeres; a méretet nem találjuk ki. A no-stub itt ROM-olvasást választ, lassabb lehet; before/after no_reset nem ad automatikus boot/reset eljárást. A dokumentált parancsok **nem lettek futtatva**, nincs flash_id/read_flash/portnyitás/reset. A mentés fájlméretét a detektált mérettel összevetni és lehetőség szerint második olvasás hash-ével ellenőrizni kell az első írás előtt. Nincs write_flash/erase_flash/bond-reset parancs vagy automatikus upload-script.

## Megőrzés és folytatási ellenőrzőpont —6/8 kész

Új tartós mentés **.local-snapshots/step-6-build-20261007/**: végső forrásarchive+manifest, firmware.bin/ELF/build-info, bootloader/partitions/framework boot_app0, artifact-plan.json, verbose hibás és normál sikeres buildnapló, offline image-info/compilerflags, checks és SHA256SUMS. Build **54adbb04939d-dirty-src98d9ef8fee3f**, UTC **2026-10-07T14:53:04Z**. Bin SHA256 **061e0e3765704e630e980612eb20d867ca6464eb8bf28fd1e87a0fca0bfe0020**; ELF SHA256 **c3abb64069f228a15c9d49adf01c52a1da705df8de9ec6e964d4d9628639fca6**. Minden korábbi mentés történeti és változatlan; az eredeti firmware továbbra is csak a teszthardveren van, ez az új rekonstruált firmware mentése.

**Elkészült:** célzott BLE-only konfigurációzárás és portválasztás, régi uploader megjelölése, változatlan runtime/paraméterek, sikeres C3 és0/0 méretdelta, statikus partíció/artifact/offset ellenőrzés, verzióhelyes mentési parancsok és a7 minimum bemenetei. **6/8-ban nincs további munka.** A fent felsorolt port/bootmód/flash/bekötés/régi-dump/otadata/NVS-host állapot csak felhasználótól vagy hardverről szerezhető meg; most nem igazolt.

Új session: aktuális első sor és lezárt6/8 szakasz, majd teljes staged/unstaged/untracked/submodule ellenőrzés; a forrás–bin–ELF pár a step-6 mentésből. Az5/8 runtime ellenőrzéseinek eredménye továbbra is történeti PASS, nem futott új teszt. **7/8 és hardverpróba nem kezdődött el; nincs portnyitás/feltöltés/reset/flash- vagy NVS-törlés, commit/push/publikálás.**


**Utolsó megőrzési ellenőrzőpont:** csak a fent jelzett4 fájl módosult; a45 baseline forrásfájlból a többi byteazonos, beleértve minden runtime/test/paraméterfájlt és AUDIT-ot. Git-index és a történeti PROGRESS az aktuális első sor kivételével változatlan; a korábbi58 mentett fájl SHA256 szerint megőrizve. Mindhárom referenciarepo/CD tiszta. Board/flash/partíció/platform/framework/package/dependency/hook és upload/monitor sebesség statikusan összevetve, megőrizve; a régi uploader teljes eredeti törzse is megmaradt. Végső forrásfingerprint98d9ef8fee3f, bin/ELF és extra image-ek együtt, source archive/manifest byteellenőrizve; git diff --check és cached ellenőrzés exit0. **6/8 kész; megállunk.**


# Felújítás —7/8-A: első írás offline terve és régi flashmentés

**Kiinduló ellenőrzőpont:** alkalmazható AGENTS nincs; AUDIT, lezárt6/8, teljes staged/unstaged/untracked állapot és submodule áttekintve. A két korábbi staged gitlinktörlés megmarad, mindhárom referenciarepo és CD tiszta. Az index/diff,38 gyökérbeli fájl és78 korábbi mentett fájl hash-e a `/tmp/keyboard-step7a-20261007/` könyvtárban rögzítve; a6/8 tartós forrás/bin/ELF mentés megőrzött.

**Offline eredmény:** a változatlan framework boot_app08192byte-os OTA-adat, két4096byte-os szektorban32byte-os rekorddal, FF kitöltéssel. A rekordok seq1/0, stateFFFFFFFF/FFFFFFFF, CRC4743989A/FFFFFFFF; mindkét CRC helyes, IDF4.4.7 szabály szerint a nagyobb seq1 választódik, két slotnál `(1−1)%2=0`: app0. A régi készülék bootadata ettől még ismeretlen. A framework E000 címe helyett a jelenlegi custom.csv által feloldott D000 az otadata eleje; változatlan byteokkal ez pontosan[D000,F000), nem érinti phy_init-et.

A platform6.13 a FLASH_EXTRA_IMAGES offseteket a post hook előtt az UPLOADERFLAGS-ba is bemásolja; ezért a projektbeli `scripts/prepare_flash_plan.py` mindkét helyen, kizárólag a framework boot_app0 párját korrigálja. A partíciókhoz a pinned framework saját parserét használjuk; nincs csomagcache-patch, új bootadat, runtime/dependency/layout változás. Bin elkészültekor ugyanaz a hook a valódi partícióbin, uploader-párok, írási/törlési tartományok és embedded ELF SHA alapján artifact-plan.json-t készít; ez ellenőrzés, nem upload-script.

Források: [Arduino2.0.17 builder](https://github.com/espressif/arduino-esp32/blob/2.0.17/tools/platformio-build.py#L209), [platform6.13 esptool extra image-ek](https://github.com/platformio/platform-espressif32/blob/v6.13.0/builder/main.py#L480), [IDF4.4.7 CRC/érvényesség/választás](https://github.com/espressif/esp-idf/blob/v4.4.7/components/bootloader_support/src/bootloader_common_loader.c#L37), [slot mapping](https://github.com/espressif/esp-idf/blob/v4.4.7/components/bootloader_support/src/bootloader_utility.c#L363), [PlatformIO post hook](https://docs.platformio.org/en/latest/projectconf/sections/env/options/advanced/extra_scripts.html).

**Hardverág:** pyserial list_ports passzív felsorolás0 port; /dev/serial/by-id, ttyUSB/ttyACM nincs. Portnyitás, esptool chip/flash azonosítás, reset vagy flasholvasás nem történt. A hardver jelen környezetből nem hozzáférhető; nincs mentés vagy mérési eredmény.

**Pontos folytatás:** szükséges normál C3 build, actual uploader-párok és kész artifactok/range/erase/hash/ELF ellenőrzése, tartós forrás–bin–ELF mentés; review-ra kész feltételes7/B terv és hiányzó hardverbemenetek. Nincs upload target vagy írás.


## Offline ellenőrzés lezárva —7/8-A

Az egyetlen szükséges normál C3 build **SUCCESS,exit0,7,89s**; a projektből bekötött post hook ténylegesen lefutott, az uploader-párok és a manifest egyeznek. Nem indult upload target. Új, üres buildkönyvtárban fordult, a meglévő package/libdeps cache-sel; nincs letöltés/frissítés. Nem ismételtünk változatlan runtime-tesztet, reference/Docker/CI buildet vagy teljes auditot; külön izolált projektmásolat buildje nem futott, a jelen részlépés szükséges buildje és offline artifactellenőrzése futott.

```sh
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step7a-20261007/build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step3-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
# Mindkettő offline, exit0; nem eszközazonosítás:
/tmp/keyboard-step1-20261006/venv/bin/python /tmp/keyboard-step3-20261007/core/packages/tool-esptoolpy/esptool.py --chip esp32c3 image_info --version 2 /tmp/keyboard-step7a-20261007/build/esp32c3/firmware.bin
/tmp/keyboard-step1-20261006/venv/bin/python /tmp/keyboard-step3-20261007/core/packages/tool-esptoolpy/esptool.py --chip esp32c3 image_info --version 2 /tmp/keyboard-step7a-20261007/build/esp32c3/bootloader.bin
```

Verziók változatlanok: Core6.2.0; espressif32@6.13.0; framework3.20017.241212+sha.dcc1105b = Arduino2.0.17/IDF4.4.7; RISC-V8.4.0+2021r2-patch5; esptool2.41100.0/**4.11.0**; SCons4.41101.0/4.11.1; NimBLE2.5.1 commit5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb; CD1.0.2 gitlinkaa6959c25a9ad7160fd515f73d8727aa86fc1ffd. Flash **516564 /1027200**, statikus RAM **25684 /327680**, delta6/8 és5/8 **0/0 byte**. Hat korábbi upstream NimBLE warning, saját forráshiba nincs.

### Konkrét új artifact/offset tábla

Minden intervallum **[kezdő,vég)**, a vég kizáró. A törlési egység4096byte, a valódi esptool4.11 tartomány-számítása alapján; ez a későbbi célzott image-írás által érintett szektorokat jelenti, nem most végrehajtott törlést. Nincs teljes flash törlés vagy gap-fill image. [Esptool4.11 írási/törlési határok](https://github.com/espressif/esptool/blob/v4.11.0/esptool/cmds.py#L524).

| Fájl | Méret byte | SHA256 | Tervezett írási tartomány | Cél | Érintett törlési szektorok tartománya /darab |
| --- | ---: | --- | --- | --- | --- |
| `bootloader.bin` | 13248 | `11af6581aaa479250e86cb7fba469fecc49f90f398992aba397ef3440f2185c0` | `[0x0,0x33C0)` | bootloader region | `[0x0,0x4000)` /4 |
| `partitions.bin` | 3072 | `0ac00d553e81c1673f62275d560989bd2d0295e353f536cf28f3cc11562ff79e` | `[0x8000,0x8C00)` | partition table | `[0x8000,0x9000)` /1 |
| `boot_app0.bin` | 8192 | `f94c5d786a7a8fab06ac5d10e33bf37711a6697636dc037559ea19cc410a17f0` | `[0xD000,0xF000)` | otadata | `[0xD000,0xF000)` /2 |
| `firmware.bin` | 542432 | `b5edb61698b10f735132aeb384ef91a3863db727edf6b3f0b7f2d0b75a753767` | `[0x10000,0x946E0)` | app0 | `[0x10000,0x95000)` /133 |

A bootloader C3-on0x0-tól indul; a partíciós tábla0x8000-tól. A két első image célja a partíciók előtti boot/table terület, nem app-partíció. A bin542432byte, ezért **484768byte** marad az app0-ban; a513/516k PIO flashszám nem binfájlméret. A méret az app1-be is beleférne, de **nem írunk app1-be, nincs OTA**. A törlési tartományok nem fedik egymást, az NVS[9000,D000), PHY[F000,10000) és app1[110000,1E0000) az **új layout szerint** érintetlen. Az alkalmazás utolsó törlési szektorának bin utáni része[946E0,95000) szintén törlődne; ennek nincs megőrzési ígérete. A tényleges régi layoutra ugyanez mentés nélkül nem állítható. A tábla vég1E0000, statikusan belefér a deklarált2MB-ba, nem fizikai kapacitásmérés.

A frozen bootloader/partitions/boot_app0 byteazonos a6/8-cal; csak a boot_app0 **címe** változott. Mindkét ESP32-C3 image checksum/digest helyes; headerek továbbra2MB/80MHz/DIO, a build preset QIO. A framework SDK rollback-beállítása megmaradt; az initializer mindkét state-je **UNDEFINED**, nem NEW/PENDING_VERIFY. A seq1→ota0 a rögzített IDF algoritmus szerinti első bootválasztás, nem hardveren igazolt indulás; hibás appnál a bootloader más slotot is próbálhat. A régi app1-ről semmit nem állítunk. [IDF4.4.7 bootválasztás és fallback](https://github.com/espressif/esp-idf/blob/v4.4.7/components/bootloader_support/src/bootloader_utility.c#L304), [partíciók](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32c3/api-guides/partition-tables.html).

**Azonos build:** `54adbb04939d-dirty-srcd7d83a80c1d9`, UTC `2026-10-07T15:10:01Z`; `firmware.elf` SHA256 `1551daefdf82ee51a37b31c15ccf76303a3ae0e40bf7cb72cb097027f3229a3d`. A bin0xB0 mezőjében a teljes ELF SHA egyezik, és a bin tartalmazza ezt a build-ID/UTC-t. A SDK app-description régi „arduino-lib-builder”/2024 dátuma nem az aktuális firmwareazonosító; ehhez a saját BOOT/build-info a mérvadó. Az ellenőrzési JSON és image-info logok a tartós mentésbe kerülnek.

**Ellenőrzőpont:** az új konfiguráció tartományhelyes offline terve kész. A soros portlista0, nincs valódi flashmentés és régi-layout vizsgálat. Következő: dokumentált read-only mentési menet, régi-dump vizsgálati bemenetek, feltételes7/B terv, végső tartós forrás–bin–ELF és megőrzésellenőrzés.

## Hardverolvasás — nem végrehajtva

**Hiányzó fizikai bemenetek:** ellenőrzött adapter típusa/jelszintje és pontos portja; táp/közös GND és valós UART bekötés; kézzel elérhető ROM-download mód/EN-boot eljárás; konkrét ESP-C3-12F jelölés/flashváltozat. A mux és a strapping pinok terhelését is a valós bekötés alapján kell ellenőrizni, nem a kódból kitalálni. Ha több port van, az adaptert a felhasználó azonosítja. Most a passzív felsorolás0 port, ezért ezek megszerzése nem állította meg az offline munkát.

Később **először kézzel** ROM-download módba kell állítani a modult az ellenőrzött eljárással; before/after no_reset nem teszi ezt helyettünk. Egyetlen tudatos csatlakozás: timeout vagy hiba után megállás és a fizikai helyzet ellenőrzése, nem automatikus reset-loop. A4.11 help/parser és ROM read út a rögzített csomagban ellenőrzött; nincs5.x kötőjeles syntax átvéve. [Esptool4.11 parser/ALL](https://github.com/espressif/esptool/blob/v4.11.0/esptool/__init__.py), [ROM read implementation](https://github.com/espressif/esptool/blob/v4.11.0/esptool/loader.py).

A következő csak **kézzel indítható, read-only mintaparancs**; most nem futott. Privát, tartós mentési mappa a már Gitből kizárt `.local-snapshots/hardware-backup-IDOPONT/`, jogosultság700; dumpok600, umask077. Git-kizárást ellenőrizni kell; tartalmat/NVS kulcsokat nem kell kiírni vagy publikálni. A helykitöltők valós adat nélkül nem használhatók.

```sh
KBD_PYTHON=/ellenorzott/build-venv/bin/python
KBD_ESPTOOL=/ellenorzott/core/packages/tool-esptoolpy/esptool.py
KBD_PORT=ELLENORZOTT_PORT
KBD_BACKUP_DIR=/privat/tartos/hardware-backup-IDOPONT
umask 077
# A mentési könyvtár privát, új és Gitből kizárt legyen.
"$KBD_PYTHON" "$KBD_ESPTOOL" --chip esp32c3 --port "$KBD_PORT" --baud 115200 --before no_reset --after no_reset --no-stub flash_id
# Csak egyértelmű JEDEC/felismert teljes kapacitás után, nem a projekt 2MB-jából:
"$KBD_PYTHON" "$KBD_ESPTOOL" --chip esp32c3 --port "$KBD_PORT" --baud 115200 --before no_reset --after no_reset --no-stub read_flash 0x0 ALL "$KBD_BACKUP_DIR/full-flash-1.bin"
"$KBD_PYTHON" "$KBD_ESPTOOL" --chip esp32c3 --port "$KBD_PORT" --baud 115200 --before no_reset --after no_reset --no-stub read_flash 0x0 ALL "$KBD_BACKUP_DIR/full-flash-2.bin"
wc -c "$KBD_BACKUP_DIR/full-flash-1.bin" "$KBD_BACKUP_DIR/full-flash-2.bin"
sha256sum "$KBD_BACKUP_DIR/full-flash-1.bin" "$KBD_BACKUP_DIR/full-flash-2.bin"
cmp --silent "$KBD_BACKUP_DIR/full-flash-1.bin" "$KBD_BACKUP_DIR/full-flash-2.bin"
```

Mindkét méretnek a flash_id által egyértelműen detektált **teljes** bytekapacitással kell egyeznie, a hash és byteösszevetés is egyező legyen, ugyanazon ROM-állapotban/köztes app-boot nélkül. A csatlakozási chip/revision és flash_id eredmény logban mentendő, dumpbyteok nélkül. Védett/olvashatatlan/eltérő dump vagy ismeretlen kapacitás esetén **hardverág megáll**; nincs --force, SPI/GPIO felülírás, védelemkiiktatás, eFuse-módosítás vagy resetpróbálgatás. Az ALL-detektálás nem helyettesíti a kapacitás egyértelmű ellenőrzését. A teljes dump nem tartalmazza a host pairing-adatbázisát vagy eFuse-okat, és nem pótol ELF-et/forrást.

**A régi dump offline vizsgálata hátravan, mert nincs dump:** C3 bootloader/image metaadatok és integritás; tényleges régi partíciós tábla és appslotok/image-ek; mindkét régi otadata rekord CRC/state/seq és kiválasztott slot az azonosított régi bootloader szabálya szerint; NVS/PHY/egyéb adatpartíciók byteintervallumai és a fenti törlési tervvel vett metszetek. Nem feltételezzük, hogy a régi tábla0x8000-nél vagy a régi NVS0x9000-nél van: az image/bootloader és hiteles táblabejegyzésekből kell azonosítani. A mai IDF OTA-algoritmus nem bizonyítja a régi bootloader verzióját vagy implementációját. A NVS tartomány/paging-formátum felismerése nem azonos a régi NimBLE/bond-schema igazolásával. Csak metaadat/határ/érvényesség eredmény dokumentálandó, nem a személyes dump tartalma.

## Review-ra kész7/B terv — konkrét új image-ek, függő régi-hardver döntés

1. **Írás előfeltétele:** hozzáférhető és fizikailag ellenőrzött C3/adapter/táp/bootmód; két egyező teljes mentés; igazolt flashkapacitás és a2MB/80MHz/QIO-preset használhatósága; régi boot/layout/otadata/NVS tartományok offline azonosítása. Biztonsági/olvasási korlátnál nem használunk felülíró kapcsolót. A régi adatok és az új törlési tartományok metszetét a mentésből review-zni kell. A mai új image-terv helyessége nem automatikus régi-layout migrációs jóváhagyás.
2. **Konkrét jelölt írás:** kizárólag a tartós step-7a mentés fenti SHA256-jaival azonos **bootloader.bin@0000, partitions.bin@8000, boot_app0.bin@D000, firmware.bin@10000**. ELF/build-info csak archiválandó. Négy külön image, nem teljes flashre padolt merged bin; nincs gap-fill/erase-all. A boot/table/otadata és az app0 fenti szektorai módosulnak. A régi firmware sértetlen megőrzése ezen a hardveren nem lehetséges, mert felülírnánk; a privát teljes dump a visszaállítás alapja.
3. **Megőrzés:** az új layout NVS[9000,D000), PHY[F000,10000), app1[110000,1E0000) és a többi nem érintett szektor az íráskor megmarad. Ezt a **régi** adatokra csak azonosított kompatibilis layout esetén lehet vállalni. Eltérő régi NVS/adat/boot terület vagy ismeretlen schema esetén nincs automatikus másolás, partíciócsere vagy NVS-migráció: külön review döntés szükséges, addig nincs első írás. A fizikai flash2MB-nál nagyobb kapacitása nem ok az új layout automatikus bővítésére.
4. **Indítás:** az otadata mindkét szektorának tudatos inicializálása a változatlan framework adatával seq1→app0@10000 választást ad. Nem hagyjuk meg az egyik régi magasabb seq-jű otadata rekordot; ezért kell a teljes8KiB, nem részleges rekord. Sikeres későbbi írás/verify és a NVS/pairing döntési pont után kézi normál boot,115200 capture. BOOT build-ID pontosan a fenti7A; eltérő image/slot esetén megállás. Nincs automatikus reboot/recovery script.
5. **Párosítás/NVS külön döntési pont az első boot előtt:** az írási terv nem töröl NVS-t vagy host-párosítást, és nem adunk új bondtörlő/migráló mechanizmust. **A jelen baseline azonban nem garantál törlésmentes runtime-ot:** Arduino2.0.17 initArduino bizonyos NVS hibáknál (`NO_FREE_PAGES`, `NEW_VERSION_FOUND`) teljes NVS-partíciót formáz; NimBLE2.5.1 `REPEAT_PAIRING` meglévő peer bondját törli és retry-t kér. Ezek változatlan upstream viselkedések, most nincs policycsere. Ezért a régi NVS/bond formátum és állapot tisztázása nélkül a7/B első boot/pairing része nem tekinthető megőrzésre jóváhagyottnak. Kezdetben a host meglévő bejegyzésével reconnectet próbálnánk, nem „forget/re-pair” automatikát; kudarc esetén leállás és naplóreview. Ha a követelmény minden automatikus runtime bond/NVS törlés kizárása, azt a jelen forrás nem teljesíti: külön következő utasításban célzott policy-döntés/javítás kell, **nem ezen7A alatt**. A korábbi bondok1.x/2.x kompatibilitását nem állítjuk. [Arduino NVS-init](https://github.com/espressif/arduino-esp32/blob/2.0.17/cores/esp32/esp32-hal-misc.c#L249), [NimBLE2.5.1 repeat-pairing](https://github.com/h2zero/NimBLE-Arduino/blob/2.5.1/src/NimBLEServer.cpp#L608).
6. **Visszaállítás:** ugyanazon fizikailag azonosított modul ROM-módjában a hiteles teljes régi dumpot0x0-tól a **mért teljes kapacitásig** visszaírni egy később külön engedélyezett műveletben, utána readback/hash és kézi boot. Ez bootloader/table/otadata/app/NVS-t együtt állítana vissza, nem csak az appot. Most ilyen parancsot vagy scriptet nem futtatunk/készítünk. Védelem/titkosítás/eFuse vagy hardverhiba miatt a mentés megléte sem garantál visszaállíthatóságot; nem változtatunk eFuse-t. A host bejegyzései ettől nem állnak vissza, és a teszt közbeni új pairing után eltérhetnek; minden host esetén külön állapot/napló. A visszaállítás nem az eredeti firmware forrásának reprodukciója.

**Rövid hardveres elfogadási sor7/B után:** helyes BOOT/10s heartbeat/scan+heap; magyar karakterek/AltGr és bal-jobb modifier; tartott normál+modifier reconnectkor elnyomott, stabil release+új press után aktív; néhány disconnect és host sleep/wake; gyors gépelés/press-release, beragadt vagy elvesző gomb megfigyelése. Windows/Linux/Android mind **NOT TESTED** jelenleg; csak ténylegesen elérhető hoston tesztelünk, OS/verzió/kiosztás/log+build-ID és eredmény külön rögzítve. Nincs hosszú stabilitáspróba vagy RF/tápmérés; megfigyelt tünet után külön célzott munka. A helyi notify jelzés nem hostvétel-bizonyíték.

## Tartós mentés és folytatási ellenőrzőpont —7/8-A offline kész

Mentés: **`.local-snapshots/step-7a-build-20261007/`**, meglévő helyi Git-kizárás alatt. Végső forrásarchive+manifest (új hookkal), firmware.bin/ELF/build-info/header, bootloader/partitions/boot_app0, artifact-plan.json, buildlog/image-info/offline checks, bemeneti index/diff/forrás- és korábbi mentéshash-lista és SHA256SUMS együtt. A6/8 és minden régebbi mentés megmarad. Ez **új rekonstruált firmware mentése**, nem régi hardverdump. A részletes artifactterv a fent megadott buildhez tartozik, a cache későbbi megléte nem szükséges az archivált image-ek ellenőrzéséhez.

**Változáslista:** `platformio.ini` extra_scripts: egy post hook bekötése; új `scripts/prepare_flash_plan.py`: tartós boot_app0-offsetkorrekció és offline artifactmanifest; `README.md`: aktuális helyes címtér/mentéshely/első-írási korlát; `PROGRESS.md`:7A checkpoints/táblák és feltételes7B. Sem runtime, dependency, partíció/pinout/flashparaméter, sem előző mentés nem változik.

**Elkészült:**7A offline boot_app0 elemzés és legkisebb projektbeli címkorrekció; sikeres C3/0-0 méretdelta; konkrét méret/hash/range/erase/slot/ELF ellenőrzés; hardverhozzáférés passzív vizsgálata; read-only mentési menet és review-képes feltételes első-írási terv. **Nincs hozzáférhető hardver: valódi flasholvasás, régi-partíció/slot/NVS vizsgálat és7/8 hardveres elfogadás hátravan. Feltöltés/első flashírás nem történt.**

**Pontos folytatás:** lezárt7A és frozen artifactterv + teljes staged/unstaged/untracked/submodule ellenőrzés; először adapter/port/jelszint/táp/ROM-boot bemenetek, majd flash_id és kétszeres teljes read-only mentés, metadata/layout/NVS átfedés elemzés. A NVS-init/repeat-pairing megőrzési korlátot a7B boot előtt külön rendezni kell. Ebből véglegesíthető a hardverspecifikus7B írási terv; firmwareírás csak külön következő felhasználói utasításra, soha vak uploadból. Nincs commit/push/publikálás; most megállunk az első írás előtt.


**Végső megőrzési ellenőrzőpont:** a3 korábbi fájl (INI/README/PROGRESS) és1 új hook az egyetlen7A forrásváltozás. Az index byteazonos, a történeti PROGRESS teljes törzse megmaradt, mind78 korábbi mentett fájl SHA egyezik; AUDIT854dac7b… és minden runtime/layout/dependency/paraméter/teszt változatlan. Mindhárom referenciarepo és CD tiszta, ugyanaz a gitlink. A forrásfingerprintd7d83a80c1d9 egyezik a tényleges builddel; az aktuális dokumentáció nem része a runtime fingerprintnek. A végső46 fájlos source archive/manifest byteellenőrzése és mind22 mentett fájl artifact/SHA256SUMS ellenőrzése PASS; git diff --check és cached ellenőrzés exit0. **7A offline kész; hardverolvasás és első írás előtt megállunk.**


# Felújítás —7/8-B offline: opcionális akkumulátorszint-közlés

**Kiinduló ellenőrzőpont:** alkalmazható AGENTS nincs; AUDIT és lezárt7/A áttekintve. Teljes staged/unstaged/untracked/submodule állapot rögzítve; a46 forrásfájl byteazonos a7A mentéssel, mindhárom referenciarepo/CD tiszta. A két staged gitlinktörlés és100 korábbi mentett fájl megőrzendő. Bemeneti index/diff/manifest a `/tmp/keyboard-step7b-20261007/` alatt. Portkeresés sincs, a felhasználó új döntése szerint minden hardverpróba elhalasztott.

**Feltárás:** KeyboardHid::begin fix80%-ot állít; nincs ADC/akkumulátor/feszültség-/fuel-gauge mérési út a src/include alatt. A NimBLE2.5.1 HID constructor automatikus180F szolgáltatást és READ/NOTIFY2A19-et hoz létre, önmagában nem ad valós mérési értéket. A stabilizált ESP-táp nem battery-százalék. Alapértelmezésben a szolgáltatás elrejtve marad, nincs kitalált szint.

**Választott út:** kicsi cached BatterySource határ (százalék + millis-időbélyeg + érvényesség), loopban ritka frissítés, saját battery link/subscription és egyetlen latest-value pending állapot; nincs keyboard-FIFO vagy mérési task. 0–100% egybyte-os GATT érték; új battery subscription/reconnect után aktuális érték, változatlan érték nélkül folyamatos notify nincs. Ismeretlen/hibás/stale cache nem értesíthető, olvasása szabványos ATT Unlikely Errorral visszautasítandó, nem sentinel/üres százalék/utolsó stale érték.

A2.5.1 onRead void és a Server::handleGattEvent mindig az attribútumot másolja; ezért egy **két upstream fájlt érintő opcionális read-guard patch** kerül a projekt scripts könyvtárába. A default-null guard minden más karakterisztika viselkedését megőrzi; csak battery read kér guardot. Buildkor a pontos pinned Git-forrásra ellenőrzötten alkalmazott, követett patch, nem cache-only javítás vagy új könyvtárverzió; upstream LICENSE/NOTICE érintetlen. A library módosított baseline-ját és a patch hash-ét rögzítjük.

Elsődleges források: [BAS1.1 §3.1](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/BAS_v1.1/out/en/index-en.html), [NimBLE2.5.1 HID](https://github.com/h2zero/NimBLE-Arduino/blob/2.5.1/src/NimBLEHIDDevice.cpp), [read API](https://github.com/h2zero/NimBLE-Arduino/blob/2.5.1/src/NimBLECharacteristic.h), [GATT access](https://github.com/h2zero/NimBLE-Arduino/blob/2.5.1/src/NimBLEServer.cpp#L723). A szolgáltatás implementálása nem host Bluetooth UI-n megjelent százalék igazolása.

**Pontos folytatás:** tracked read-guard + BatterySource/state/adapter, rövid native trace-ek, normál C3 és izolált compile-fixture build (normál firmware-ben nincs tesztforrás), README/PROGRESS, új forrás–bin–ELF tartós mentés. Sem feltöltés, sem7A boot/layout/NVS-policy változtatás nincs; 8/8 publikálási audit/MIT külön következő prompt.


**7B ellenőrzőpont / implementáció:** source/sample/read-guard/state/adapter elkészült. Source=nullptr alapértelmezés: a NimBLE által automatikusan létrehozott BAS az első regisztráció után removeService(false), majd **kizárólag advertising előtti** server-starttal kikerül a GATT táblából; nem fut reconnectenként stack-reset. Enabled backend esetén ugyanez a180F/2A19 marad, per-attribute read-guarddal és külön callbacks/CCCD állapottal. Az adatot callback csak védett másolatból olvassa, soha nem mér.

BatterySource::cachedSample csak loopban, induláskor és legfeljebb1000ms-onként; a mérési timestamp nem az olvasás idejére cserélendő.60s kezdeti maxAge,1000ms notify/retry térköz (munkamenetváltáson át is); konfiguráció include/BatteryState.h, nem bemért értékek. A guard üres/hibás/stale mintánál ATT0x0E hibát ad, így az előző attribútumérték sem kerül ki. Érvényes0% valós teljes lemerülés, nem ismeretlen jelzés. A callback és loop között csak a kis sample/link másolatot védi a linkMux; nincs GATT/notify/Serial a kritikus szakaszban.

Native ASan/UBSan battery trace PASS, első normál C3 SUCCESS518304/25772, izolált fixture SUCCESS518512/25772. A fixture könyvtár két érintett fájlja eredeti pinned upstreamre visszaállítva volt, a patch újra alkalmazódott; nincs .pio/secret/ kézi pótlás. A végső review során a build-fingerprint az aktív fixture headerét is felveszi, ezért a két végső artifactot új azonosítóval szükséges inkrementálisan előállítani. Ez metadata-korrekció utáni build, nem változatlan ellenőrzés ismétlése; korábbi eredmények/logok megmaradnak.

**Folytatás:** végső két build, érintett LED-read regresszió, rövid README/hiányzó backend adatok és7 offline lezárás, két elkülönített source/bin/ELF/hash mentés. Runtime session/debounce/descriptor/security/partíció/pinout és7A offsethook érintetlen.


## Lezárt battery-közlés és ellenőrzések —7/8-B offline kész

**Forrás és közlés külön:** `include/BatteryState.h:4–29` BatterySample/BatterySource/freshness határ; `src/BatteryState.cpp` legújabb állapot és helyi elfogadás külön, konstans memória. A normál `src/main.cpp` pointer nullptr, tehát **nincs mérési backend vagy engedélyezett akkumulátorközlés**. Valódi backend bekötése egy BatterySource és a normál pointer beállítása; nincs új sensor-framework, ADC/pin/osztó/akku/töltő feltételezés.

**GATT:** `src/KeyboardHid.cpp:45–94` kezeli az automatikus BAS-t. Default esetben advertising előtt kivezetett180F szolgáltatás, nem üres/80/100%-os attribútum. Nem cseréltünk HID-descriptort vagy stack-et. Enabled esetben a pinned HID Battery Level READ/NOTIFY2A19/2904 és a NimBLE által kezelt CCCD marad. `batteryReadGuard` (`:102`) friss0–100 mintánál egybyte-os értéket ad, invalid/unknown/stale mintánál ATT Unlikely Error0x0E-t. A helyi char tárolójában maradó régi érték sem olvasható stale adatként. Host UI az előzőleg kapott értéket ettől még cache-elheti: annak megjelenése/eltűnése és a BAS nélküli HID-profil viselkedése **hardveren ellenőrizetlen**. Nem állítunk Bluetooth minősítést vagy teljes HOGP-konformanciát a battery nélkül.

A BAS nem definiál saját ismeretlen százalékot/application error kódot; az ATT read-kérés szabványos hibát kap, nem speciális hostprotokollt. A százalék típusa/range és változás/reconnect értesítés alapja a fentiBAS1.1 §3.1; a0x0E a tényleges pinned `src/nimble/nimble/host/include/host/ble_att.h:102` konstanssal fordul. [Upstream ATT hibakód](https://github.com/apache/mynewt-nimble/blob/master/nimble/host/include/host/ble_att.h), [NimBLE2.5.1 service visibility](https://github.com/h2zero/NimBLE-Arduino/blob/2.5.1/src/NimBLEServer.cpp#L819). A konkrét2.5.1 ble_att.h URL webes lekérése cache-miss hibás volt; a helyi pinned header az API bizonyítéka, az Apache upstream link a standard kód háttere, nem más stackverzió választása.

**Kapcsolat/küldés:** `pollBattery` (`src/KeyboardHid.cpp:114`) offline is frissítheti a forráscache-t. BatteryLink saját generáció/handle/encryption/subscription; connect/disconnect, encryption-váltás és battery CCCD változás új aktuálisérték-közlést indokol. Keyboard CCCD nem resynceli, battery CCCD nem módosít keyboard readiness/generációt (`:214–221`). Notifyhoz GAP+encrypted+battery subscription kell, **keyboard subscription nem**. Read a BAS eredeti READ engedélyével elérhető, a keyboard security igényét nem gyengítettük. A tetszőleges1–100% köztes érték nem ASCII/HID-report.

A cache1000ms-onkénti pollja és notify/retry1000ms minimum külön korlát, a rate limit generációváltáson át megmarad. Százalékváltás és resubscribe/reconnect indít függő közlést, változatlan lokálisan elfogadott érték nem ismétlődik. Helyi elutasítás korlátos gyakorisággal újrapróbálható, új mérés felülírja az elutasított régi battery-értéket; nincs eseménysor vagy végtelen ciklus. Missing/invalid/stale sample esetén nincs notify-próba. A forrást loop-only használjuk, a callback csak a sample/link védett másolatát olvassa. Critical szakasz nem fog át GATT/notify/Serial hívást. Keyboard submit előbb fut, battery-hiba nem nyúl a keyboard session/FIFO/counter/completion útjához.

Notify előtt/után battery generációellenőrzés, régi eredmény nem teljesít új linket. A rögzített handle-only notify API check→notify és handle-reuse versenye itt is marad; a késői battery TX-státusz nincs reporthoz rendelve, és nem teljesít HID/BatteryState pendinget. A bool csak helyi elfogadás; read/notify integráció vagy hostvétel nem igazolt.

### Tartós, verzióazonosított library patch

A `scripts/nimble-read-guard.patch` a2.5.1 két fájljához10 hozzáadott sor: optional ReadGuard/setter+null mezők a NimBLELocalValueAttribute-ban, majd guard/error-return a Server GATT read ágában. Minden más attribútum guardja nullptr; default olvasás/LED write ág változatlan. Patch SHA256 **dd28eae660741104c068b69e18d13bb87c9299ef29c1772d24f177786f9bdec7**. `scripts/nimble-read-guard.json` pontos előtte/utána hash-ek, a `prepare_build.py` mindkét fájl byteállapotát és a full upstream HEAD-et ellenőrzi, majd git apply --check/apply. Már patchelt állapot idempotens, eltérő/kevert kézi módosításnál hibával megáll és megőrzi azt. Az upstream Git rev változatlan, **a buildelt NimBLE kód most ezzel a dokumentált patch-csel módosított**, nem tiszta upstream byteállapot. Ez nem dependency-frissítés. A projektben követett patch/script/JSON együtt reprodukálja, cache törlése után is.

Az upstream Apache-2.0 LICENSE/NOTICE és a módosított fájlok jogi fejlécei változatlanok. A patch nem új licenc a régi HUN-eredetű kódra; nem adtunk MIT-licencet ebben a lépésben. A patch eredeti/utána metaadata, library diff és LICENSE/NOTICE a tartós mentésben is megőrzendő; teljes publikálási/licencaudit csak8/8.

### Célzott ellenőrzések

Ténylegesen futott, PASS:

```sh
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iinclude tools/check_battery.cpp src/BatteryState.cpp -o /tmp/keyboard-step7b-20261007/check-battery
ASAN_OPTIONS=detect_leaks=0 /tmp/keyboard-step7b-20261007/check-battery
# Érintett, korábbi minimális LED-regresszió; a globális read-guard mellett write változatlan:
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g tools/check_led.cpp -o /tmp/keyboard-step7b-20261007/check-led
ASAN_OPTIONS=detect_leaks=0 /tmp/keyboard-step7b-20261007/check-led
```

Konkrét battery-orákulumok: read0/42/100; −1/101/valid=false/üres nincs adat;60s még fresh,60001ms stale; future timestamp elutasított, FFFFFFF0→00000020 fresh és60001ms wrap stale. Offline/encryption/subscription hiányában nincs attempt. Readyben42 local-reject után999ms-nál nincs retry,1000ms-nál van; local-success után változatlan42 nincs notify, majd100 és0 helyesen próbálható. Reject0 után invalid/missing/stale nem próbálkozik, friss55 a legújabb pending. Disconnect+connect ugyanazon7 handle és régi eredmény után új generation23; rate limit nem kerülhető meg reconnecttel. Unsubscribe→resubscribe24; keyboard/battery linkek egymástól külön. Notify retry időátfordulás999/1000ms; frissülés nélkül lejárt sample nem küldődik. Ezek a firmware-ben használt batteryLevel/BatteryState/BatteryLink kódot használják, az elfogadási bool vezérelt helyettesítő; nem rádió/GATT-host emuláció és nem teljes callback-interleaving vizsgálat. ASan/UBSan aktív, LeakSanitizer a korábbi sandbox ptrace korlát miatt kikapcsolva.

Az első normál build és a pristine libraryből végzett egyszeri izolált enabled build fent dokumentált köztes SUCCESS. A fixture-projekt a követett+untracked forrás és CD fájlok másolata, nincs .git/.pio/secret/kézi könyvtárpótlás; a csomagcache használható volt. NimBLE saját izolált cache másolatában csak a két érintett fájlt állítottuk eredeti pinned byteokra a másolat Git-objektumaiból, így a build bizonyította a patch újbóli alkalmazását. Csak az izolált INI-be került `-DKEYBOARD_BATTERY_COMPILE_CHECK`, megfelelő42%-os, cached tesztforrással. Normál INI/pointerben nincs tesztmérés; a fixture fejléce makró nélkül #error, az azonosító **BATTERY-TEST-ONLY**.

Végső, az aktív fixture-headerre is teljes fingerprintet készítő script után mindkét inkrementális C3 build **SUCCESS,exit0**: normál4,15s, fixture4,31s. Nincs letöltés, dependency/frissességi audit, Docker/CI/ref build vagy teljes regressziós sor.

```sh
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step7b-20261007/build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step3-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step7b-20261007/fixture-build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step7b-20261007/fixture-libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -d /tmp/keyboard-step7b-20261007/fixture-project -e esp32c3
```

| Build | PIO flash byte | Statikus RAM byte | Delta7/A flash/RAM |
| --- | ---: | ---: | --- |
| Lezárt7/A |516564|25684|—|
| **7/B normál, source=nullptr** |**518304 /1027200**|**25772 /327680**|**+1740 /+88**|
| Enabled compile fixture, nem valós backend |518512 /1027200|25772 /327680|+1948 /+88|

Normál RAM/flash növekmény a kész beköthető battery kód/állapot és read-guard; a backend és BAS normál hostnak nem aktív. A PIO statikus RAM nem teljes heap/stackmérés; a guard attribútumonként két pointert ad a heap-objektumokhoz. A két teljes első fordítás a korábbi hat upstream NimBLE warningot adta, a végső inkrementális buildekben nincs új warning; saját forráshiba nincs.

Pontos csomagok változatlanok: Core6.2.0; espressif32@6.13.0; Arduino2.0.17/IDF4.4.7, framework3.20017.241212+sha.dcc1105b; RISC-V8.4.0+2021r2-patch5; esptool2.41100.0/4.11.0; SCons4.41101.0/4.11.1; NimBLE2.5.1 commit5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb **+ fenti read-guard patch**; CD1.0.2 gitlinkaa6959c25a9ad7160fd515f73d8727aa86fc1ffd.

### Forrás–bin–ELF és folytatás

**normal**: `54adbb04939d-dirty-srca65dcbf243f3`, UTC `2026-10-07T15:42:30Z`; bin544624byte, SHA256 `a9d9ffcd34b77fb912897e6ebb73bd8cd39849f2e7043a18628585ecc05d1353`; ELF SHA256 `2d326d8d3c9b67af9a179a29b2640dbd9d66e98d22e69ea90ec2fab2ade58048`.

**compile_fixture**: `no-git-unknown-srce031c63ba5e6-BATTERY-TEST-ONLY`, UTC `2026-10-07T15:42:49Z`; bin544912byte, SHA256 `7b9921171f2bb31c4e849e1e0dbd97c49f6fc9ea350e33a9e47374d4ad1ef853`; ELF SHA256 `2b4a0c78d315b36021cab7b28a504c9edf4f0b19854c581a47603bcb2b93eb7d`.

A normál és compile-fixture binbe ágyazott ELF SHA, saját build-ID/UTC, build-info és manifest egyező, utóbbi compile_fixture false/true-val külön jelöl. A7A hook továbbra mindkét buildnél ellenőrizte az uploader/artifact tervet: bootloader0000, partitions8000, változatlan boot_app0D000, app10000; partícióbin/bootloader/initializer változatlanok, új firmware belefér app0-ba. A7A frozen image-ek/history nem felülírva; az új buildhez új hash és binvég tartozik, jövőbeli íráskor nem keverhető a7A bin/ELF-fel. Nincs új feltöltési vagy NVS-migrációs policy.

Tartós mentés **`.local-snapshots/step-7b-build-20261007/`**: normál teljes source archive/manifest, bin/ELF/build-info/header, három extra image és artifactmanifest, buildek/native ellenőrzési eredmények, bemeneti index/diff/hashes; a **compile-check/** alkönyvtárban saját source/INI/archive/manifest és saját bin/ELF/ID/offsetmanifest, egyértelmű TEST-ONLY jelöléssel. Library patch/meta/diff/LICENSE/NOTICE provenance megőrzött, SHA256SUMS együtt. A korábbi100 mentett fájl megőrzését hash-ekkel ellenőrizzük.

**Még hiányzó mérési bemenetek:** akku tényleges megléte/típusa/kapacitása; táp+töltés topológia; igazolt gauge/ADC/interfész és bekötés/jelszint; ADC-nél szabad tényleges pin/osztó/bemeneti korlátok; kalibráció és százalékmodell az adott akkura; tényleges mérési ritmus/hibajelzés. Nincs új ADC/pin, nincs fix tesztpercent normál adatforrásként.

**7/8 offline kész; hardveres elfogadás elhalasztott, nincs kipipálva.** Windows/Linux/Android százalék UI és GATT-host integráció **NOT TESTED**. Minden korábbi pairing/reconnect/debounce/ghosting/táp/RF/hosszú stabilitás hardveren ellenőrizetlen. Nem kerestünk portot, nem kértünk csatlakoztatást, nincs reset/flash_id/read/write/upload/erase/NVS/bond/eFuse művelet, commit/push/publikálás. A régi hardverfirmwarehez nem nyúltunk.

**Változáslista:** új `include/BatteryState.h`, `src/BatteryState.cpp`, `tools/check_battery.cpp`, `tools/battery_compile_fixture.h`, `scripts/nimble-read-guard.patch`, `scripts/nimble-read-guard.json`. Módosult `include/KeyboardHid.h`, `src/KeyboardHid.cpp`, `src/main.cpp`, `scripts/prepare_build.py`, `README.md`, `PROGRESS.md`. PlatformIO/dependency verziók,7A uploadhook éscustom.csv, magyar layout, GPIO/mux/20µs, debounce, keyboard session/FIFO/rollover/report, descriptor/security/TX-power/watchdog/brownout/OTA állapot változatlan.

**Pontos folytatás:** aktuális első sor és lezárt7/B offline rész, teljes Git/staged/untracked/submodule állapot, a normál source/bin/ELF mentés és patch provenance. A következő külön prompt a8/8 publikálási audit és MIT; ott az összes hardveres elfogadás elhalasztását és a harmadik féltől származó licenceket egyértelműen kezelni kell. **8/8 nem indult el.** Ha később hardverpróba következik, a7A adapter/kapacitás/két teljes flashmentés/régi layout/NVS/bond/első-boot korlátjai továbbra előfeltételek; a korábbi7A „7/B írási terv” megmaradt feltételes történeti terv, nem most végrehajtott feladat. Most nem kezdjük el sem a backend bekötését feltételezésekből, sem a hardveres feltöltést.


**Végső megőrzési ellenőrzőpont —7/8 offline lezárva:** pontosan6 korábbi forrás/dokumentáció fájl módosult és6 új fájl készült, a végső forrásmanifest52 fájl. Az index byteazonos a bemenettel; a történeti PROGRESS teljes törzse változatlan prefix. Mind100 korábbi mentett fájl SHA egyezik. AUDIT/LICENSE, INI/custom.csv/7A offsethook és a keyboard állapotkezelés változatlan; mindhárom referencia és CD tiszta. A NimBLE LICENSE/NOTICE upstream byteazonos; a két library fájl pontos utána-hash-e és a normál/fixture runtime fingerprint egyezik a tényleges buildekkel. `git diff --check` és cached ellenőrzés PASS.

A `.local-snapshots/step-7b-build-20261007/` végső normal és külön TEST-ONLY compile-check forrásarchive-ja manifest alapján byteellenőrzött; bin/ELF beágyazott hash/build-ID és minden artifact méret/SHA egyezik, a boot_app0 továbbra D000-ra tartozik. A teljes új mentés SHA256SUMS-szal ellenőrzött, helyi Git-kizárás alatt van. A build/test eredmények és a library patch provenance együtt tartósak; ez rekonstruált új firmware, nem az eredeti hardverfirmware reprodukciója vagy flashmentése. **Hardveres elfogadás nincs teljesítve; 8/8 nem kezdődött el.**


# 7/8 offline kiegészítés — LiPo ADC BatterySource

**Bemeneti ellenőrzőpont:** alkalmazható AGENTS nincs; lezárt7/B és teljes staged/unstaged/untracked/submodule diff áttekintve.52 aktuális forrásfájl egyezik a7/B tartós mentéssel; index/diff/forrás- és korábbi mentéshash a `/tmp/keyboard-step7adc-20261007/` alatt. Referenciák/CD megőrzendők; nincs portkeresés vagy hardver.

**Pin/API feltárás:** GPIO0 az első ADC1 jelölt és kódszinten szabad; GPIO1–4 a muxCol vezérlése, GPIO5 ADC2 és muxRow, nem használható. MuxRow9/8/5, muxCol4/3/2/1, mátrixjelek6/7, közvetlen oszlop18/19; UART21/20. A foglaltság közös pin-konstansokból ellenőrizendő, tényleges bekötés nem igazolt. Arduino2.0.17 HAL mV-API a nyers hibajelzést uint16-ra alakítja; az IDF4.4.7 ADC1 int eredménye és esp_adc_cal API a kisebb, ellenőrizhető út. Kalibráció-hiány nem kezelendő észrevétlen nyers skálázással.

**Folytatás:** központi alapból kikapcsolt konfiguráció, tiszta hostolható osztó/modell/hibaszűrés, loopban legfeljebb egy ADC-konverzió, cached BLE határ. Rövid native ellenőrzés és normal + külön ADC TEST CONFIGURATION build; végül README/eredmények és elkülönített forrás/bin/ELF mentés. A read-guard és a keyboard session változatlan.


**ADC implementációs ellenőrzőpont:** közös KeyboardPins konstansokból azonos bekötéssel kész a szabad ADC1 ellenőrzés, GPIO0 jelölt. A BatteryAdcConfig normál enabled=false és hiányzó hardveradatok nullák/-1-ek. BatteryAdcMath tiszta számítás/filter; BatteryAdcSource IDF1-konverziós loop-poll és cachedSource. Kalibráció TP (nem TP_FIT), a konkrét C3 v4.4.7 forrás ezt támogatja; eFuse-hiánynál no-data/fallback=none. Native ellenőrzés PASS; normal és külön ADC TEST CONFIGURATION C3 build SUCCESS. A korábbi read-guard/KeyboardHid/BatteryState/session/debounce/offsethook és dependency változatlan.

**Folytatás:** README és lezárási eredmények, inputmegőrzés/diff/forrásfingerprint ellenőrzés, normal és ADC-TEST-CONFIGURATION tartós külön archive/bin/ELF/hash. Nincs új build/teszt szükség változatlan kódra; hardverpróba és8/8 nem indul.


## Lezárt 7/8 offline ADC-kiegészítés

**Mi készült:** új `BatteryAdcSource` a7/B cached BatterySource határhoz, külön `poll()` és `cachedSample()`; új hostolható `BatteryAdcMath` az osztó/modell/filter számításaihoz; központi `BatteryAdcConfig`. A keyboard scan/session után legfeljebb egy ADC1-konverzió fut looponként, nincs új task, callbackbeli mérés, várakozó alkalmazásciklus vagy FIFO. Cached minta timestampje az elfogadott burst utolsó tényleges konverziója, nem BLE-read/poll idő. A BLE-cache legfeljebb további1s késést adhat; invalid/unknown/stale adat továbbra a meglévő read-guard ATT hibája, nem0/100 sentinel. A7/B battery/keyboard CCCD, generáció, local notify/retry elkülönítés megmarad. A read-guard patch byteazonos, nincs bővítése.

**Pinbizonyíték:** `include/KeyboardPins.h` és `src/main.cpp` ugyanazokat a konstansokat használják; muxRow9/8/5/−1, muxCol4/3/2/1, row/col6/7, direkt18/19, UART21/20. GPIO0 az egyetlen jelenlegi kódszinten szabad ADC1 jelölt; GPIO1–4 foglalt, GPIO5 ADC2/mux és tiltott. Sem virtuális128+ muxcsatornát, sem preset A0–A5 aliasokat nem tekintünk szabad fizikai bekötés bizonyítékának. Fizikai pinmux/vezeték/modulespecifikus hozzáférés ellenőrizetlen. A foglalt/ADC2/egyéb pin és hibás konfiguráció `validConfig`/`begin` elutasítást kap, az ADC-pin konfigurálása előtt. A meglévő pinértékek és matrix/layout/20µs/scanner és keyboard állapotkezelés változatlan, csak a megosztott konstansok helye új.

**Konfiguráció/policy:** `include/BatteryAdcConfig.h` normál `enabled=false`; GPIO/attenuation−1, osztó/cellaszám/max és modell-végpontok0. Nincs automatikus vagy normál firmware-tesztforrás. Tudatos engedélyezéshez meg kell adni az ellenállásokat ohmban (top=akku+→ADC; bottom=ADC→GND), cellaszámot és teljes maximális pack mV-t, megfelelő attenuationt, modell-végpontokat/plausible alsó határt és toleranciát. A szoftver1–4 LiPo-cellás és4000–4350mV/cella maximumú paraméterezést enged, ez támogatási korlát, nem azonosított akku. A packmérés nem egyedi cellamonitor/BMS.

A normál kezdeti időzítés30s/burst,25ms/minta,4 mintás aritmetikai mV-átlag, legfeljebb1000ms burst-életkor;1–16 minta és1–30s periódus validálva. Maximuméletkor a7/B60s, millis-idők unsigned difference kezelésűek.16 nyers rail-margin és100mV/cella max burst-szóródás kezdeti, nem bemért érték. Egyetlen negatív drivereredmény, rail/telített/nem értelmezhető mV, túl szórt vagy túl régi partial burst azonnal érvényteleníti a cache-t; nem átlagoljuk el a hibát. Következő mérési periódusban új burst, nincs gyors hibaretry. Kalibráció/driver-init hibánál nincs mérés és nincs begin újrapróbáló ciklus. Lebegő bemenet biztos felismerése nincs.

`V_bat=V_adc×(Rt+Rb)/Rb`, millivoltra kerekítve. Előtte a kalibrált ADC mV-ra kézi gainPpm/offsetMv korrekció lehetséges; identitás1e6/0 nem bemért kalibráció. A beállított max packfeszültséget a legrosszabb ellenállástoleranciával (Rb nő, Rt csökken) ellenőrizzük, legalább10% ADC headroom mellett. C3 mérési limit attenuation index0/1/2/3 szerint750/1050/1300/2500mV; nem abszolút elektromos terhelhetőség. A telepített IDF enum ADC_ATTEN_DB_12=3, a deprecated DB_11 ugyanaz; az Arduino2.0.17 HAL ADC_11db néven használja. [IDF4.4.7 ADC](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32c3/api-reference/peripherals/adc.html).

**Pontos API és kalibráció:** telepített Arduino2.0.17 `cores/esp32/esp32-hal-adc.c:152–253`, SDK `driver/adc.h`, `esp_adc_cal.h`, `soc/adc_channel.h`, `hal/adc_types.h` és C3 `esp_efuse_rtc_calib.h` ellenőrizve. Arduino mV-API uint16 nyers értéke helyett `adc1_get_raw` int eredmény megőrzi a negatív hibát; GPIO→channel mapping a driverrel is ellenőrzött. `adc1_config_width(ADC_WIDTH_BIT_12)` és channel attenuation hibakódot ellenőriz, `esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP)` után a calibration digit/voltage is ellenőrzött, majd `esp_adc_cal_characterize` és `esp_adc_cal_raw_to_voltage`. C3 v4.4.7 TP-only karakterizálás, nem ESP32-defaultVref vagy TP_FIT; hiányzó/rossz kalibráció esetén **no-data, fallback=none**, eFuse-írás nincs. A calibration adat előellenőrzése a karakterizálás rossz adat miatti abort/divide kockázatát csökkenti. [Arduino2.0.17 HAL](https://github.com/espressif/arduino-esp32/blob/2.0.17/cores/esp32/esp32-hal-adc.c), [C3 IDF4.4.7 kalibráció](https://github.com/espressif/esp-idf/blob/v4.4.7/components/esp_adc_cal/esp32c3/esp_adc_cal.c). Több hibás/nemlétező forrásútvonalra a web cache-miss/404 választ adott; a végül feloldott esp32c3/esp_adc_cal.c és telepített header/library a döntés alapja.

Enabled induláskor egy BATTERY_ADC status sor; státuszkódok a README-ben és `BatteryAdcSource::Status` enumában. Getterből runtime status/valid/timestamp/mV megfigyelhető, nincs konverziónkénti soros áradat. A driver szinkron oneshot hívásának tényleges futásideje hardveren nem mért; a korlát egy konverzió/loop és mintaközi idő, nem garantált hardveres real-time határ.

**Százalék:** két, cellaszámra skálázott állítható feszültség-végpont közt kerekített lineáris interpoláció0–100. Csak a plausible min…max tartományban érvényes; az empty alatt de plausible felett0%, full felett de max alatt100% modellérték, nem ADC-hibából képzett töltöttség. Példapont3300=0,3750=50,4200=100. Feszültségből becslés, nem pontos kapacitás/charge-complete; terhelés/töltés/hőmérséklet/öregedés/akkutípus befolyásolja. Nincs árammérés, töltésvezérlés vagy sleep. [TI feszültségalapú SOC korlátok](https://www.ti.com/lit/an/sluaaa1/sluaaa1.pdf#page=4).

**Fizikai előfeltételek:** tényleges LiPo cellaszám/specifikáció/legnagyobb töltőfeszültség, Rt/Rb és tolerancia, modul GPIO0 hozzáférhetősége és közös GND, táp/töltő topológia és kikapcsolt ESP melletti ADC-csomópont viselkedés még kell. A README rajza csak terv. ADC csomópont–GND bypass kapacitás (pl100nF) lehetőség, nem új bekötés; nagy osztóimpedancia és RC beállás méréssel vizsgálandó. [Espressif zajcsökkentés](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32c3/api-reference/peripherals/adc.html#minimizing-noise). Később multiméteres ADC/akku összevetés több ponton és valós terhelésen, majd gain/offset és modell ellenőrzés; most nincs bemért pontosság.

### Célzott ellenőrzések és buildek

Native PASS, tényleges firmware `BatteryAdcMath`/Filter kóddal:

```sh
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iinclude tools/check_adc_battery.cpp src/BatteryAdcMath.cpp -o /tmp/keyboard-step7adc-20261007/check-adc
ASAN_OPTIONS=detect_leaks=0 /tmp/keyboard-step7adc-20261007/check-adc
```

Konkrét elvárások:1875mV ADC→3750mV pack,2100→4200;1,02 gain és−10mV offsettel1875→3806 pack.0/hibás osztó, foglalt1–4/ADC2/invalid pin, attenuation/index/range, pack/cellaszám és modell-sorrend, túl nagy feszültség, tolerancia/headroom, idő/filter/gain hibák elutasítása.2800/3300→0%,3750→50%,4200/4250→100%,2799/4251 invalid, a teljes2800…4250mV tartomány monoton0–100.2S7500→50 konkrét math példa, nem hardverazonosítás. Negatív/0/16/4079/4095/65535 raw invalid;1860/1870/1880/1890 ADC minták átlaga3750 pack/50%, timestamp75ms. Hiba a siker után azonnal valid=false; szórt/1001ms partial burst elutasítva; FFFFFFF0→3B időátfordulásos burst pontos50%, timestamp3B.

Az első native fordításban egy hiányzó initializer_list fejléc okozott compile hibát; a test include javítva, utána fordítás és ASan/UBSan futás PASS. Nem assert/runtimehiba, firmware-kód javítása emiatt nem kellett. LeakSanitizer a korábbi sandbox ptrace-korlát miatt kikapcsolt. Nincs új tesztkeret vagy változatlan teljes regressziós sor.

Normál C3 és egyszeri izolált ADC TEST CONFIGURATION C3 **SUCCESS**. Az izolált projektben csak az INI-be került `-DKEYBOARD_ADC_TEST_CONFIGURATION`; `tools/adc_test_configuration.h` hipotetikus1S/GPIO0/100kΩ+100kΩ/index3/4250mV adatokat és10s burstöt ad. A normál konfiguráció nem használja a header értékeit. A teszt nem a7/B42%-os synthetic fixture: **valódi ADC driverkód fordul**, hardveren nem futott. A build-ID és JSON ADC-TEST-CONFIGURATION/adc_test_configuration=true jelölést kap; testheader aktív fingerprintbe tartozik. A két battery fixture egyszerre tiltott.

```sh
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step7adc-20261007/build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step3-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -e esp32c3
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step7adc-20261007/adc-test-build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step3-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -d /tmp/keyboard-step7adc-20261007/adc-test-project -e esp32c3
```

| Build | PIO flash byte | Statikus RAM byte | Delta7/B normál flash/RAM |
| --- | ---: | ---: | --- |
|7/B normál baseline|518304|25772|—|
|**ADC-kiegészítés normál, disabled**|**518328 /1027200**|**25772 /327680**|**+24 /0**|
|ADC TEST CONFIGURATION|523724 /1027200|26060 /327680|+5420 /+288|

Normál7,40s és enabled7,41s, mindkét log SUCCESS;6 korábbi upstream NimBLE warning/build, saját ADC-kód warning nincs. Normál állapotban nem történik ADC-init/konverzió, nincs kitöltött hardverkonfiguráció, a BAS elrejtve marad. A statikus RAM nem teljes runtime heap/stackmérés. Csomagletöltés/frissítés/új függőség nincs. Változatlan Core6.2.0, platform6.13.0, Arduino2.0.17/IDF4.4.7, framework3.20017.241212+sha.dcc1105b, RISC-V8.4.0+2021r2-patch5, esptool2.41100.0/4.11.0, SCons4.41101.0/4.11.1, NimBLE2.5.1@5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb+változatlan7/B read-guard, CD1.0.2@aa6959c25a9ad7160fd515f73d8727aa86fc1ffd.

### Mentés és folytatási ellenőrzőpont

Tartós mentés: **`.local-snapshots/step-7adc-build-20261007/`**, Gitből kizárt normal forrásarchive/manifest és bin/ELF/build-info/header/bootloader/partitions/boot_app0/artifact-plan/log/test+preservation checks; **adc-test/** külön saját forrás/INI/header/bin/ELF/ID/log/manifest, TEST CONFIGURATION jelöléssel. Ez új rekonstruált firmware, nem eredeti hardverfirmware vagy régi flashdump. A7/A boot_app0D000 offsetkorrekció és feltételes első-írási előfeltételek változatlanok; az új binhez új artifact SHA/range terv tartozik.

**normal disabled**: `54adbb04939d-dirty-src24361098a0f5`, UTC `2026-10-07T19:55:32Z`; bin 544656 byte, SHA256 `f3429cc2ee7dd05fc5c35ad6444c581c37f7575c9c4537de2fd06dca6caea81e`; ELF SHA256 `35b682e98c0e0c5d595374aac7b62685aa480c2f44c922b3e64b66c4acdddc22`.

**ADC TEST CONFIGURATION**: `no-git-unknown-src715c8ba22043-ADC-TEST-CONFIGURATION`, UTC `2026-10-07T19:56:04Z`; bin 550560 byte, SHA256 `3c63a299a8ff8807eb2257efa1f996f6f77e2101c7452ac0cea618a2b5b22b13`; ELF SHA256 `d1e72c87d477389ecce880b4108ed78c6b3d65f086ad5e5a167932e609d08c9b`.

**Megőrzés:**4 meglévő fájl módosult (README/PROGRESS/main/prepare_build) és8 új készült (KeyboardPins; BatteryAdcConfig/Math/Source header; Math/Source cpp; adc_test_configuration/check_adc_battery). Git-index byteazonos, teljes történeti PROGRESS prefix megmaradt,144 korábbi mentett fájl hash-e egyezik. AUDIT/LICENSE, read-guard patch/JSON/KeyboardHid/BatteryState, pinértékek/layout/session/debounce, INI/custom.csv/7A offsethook/dependencies és referenciák megőrizve. Mindkét runtime fingerprint, build-ID, binbe ágyazott ELF hash és artifactméret/hash/range terv ellenőrzött. Diff és cached diff whitespace check PASS.

**7/8 offline implementáció kész; hardveres elfogadás nincs teljesítve.** Nincs portkeresés/nyitás, hardverelérés vagy bekötés, reset, flash/NVS/bond/eFuse művelet, feltöltés, commit/push/publikálás. Az akku adatai/bekötése, multiméteres kalibráció, ADC driver latency, mérési pontosság, battery GATT/host UI, Windows/Linux/Android párosítás és tartós stabilitás továbbra NOT TESTED. A7/A teljes régi flashmentés/régi-layout/NVS/bond/első-boot korlátai fennállnak.

**Pontos folytatás:** a fenti lezárt ADC-kiegészítés és új normal/ADC-TEST-CONFIGURATION forrás/bin/ELF mentés; teljes Gitállapot áttekintése. A8/8 publikálási audit és MIT külön következő feladat, **nem indult el**. Fizikai backend engedélyezése csak megadott/ellenőrzött Rtop/Rbottom/tolerancia/cellaszám/maximum/ADC-csomópont után, a README mezőivel; a mostani TEST CONFIGURATION nem emelendő át automatikusan valós hardveradatként.


**Végső tartós ellenőrzőpont:** a60 fájlos normal és külön60 fájlos ADC TEST CONFIGURATION forrásarchive manifest szerint byteellenőrzött. A firmware/ELF/build-info/extra image/artifactmanifest mindkét esetben egyező, az új mentés SHA256SUMS-szal ellenőrzött és Gitből kizárt. Az előző144 mentett fájl és az index byteazonos maradt; a történeti PROGRESS teljes törzse megmaradt. A hardveres elfogadás elhalasztott,8/8 nem kezdődött el.


# 8/8 offline lezárás — publikálási audit és saját kód MIT

**Bemeneti ellenőrzőpont:** alkalmazható AGENTS nincs; AUDIT és lezárt ADC-kiegészítés áttekintve.60 forrásfájl egyezik a7ADC tartós mentéssel; teljes staged/unstaged/untracked/submodule állapot rögzítve. Index/diff/forrás és korábbi mentéshash a `/tmp/keyboard-step8-20261007/` alatt. Nincs hardver/portművelet.

**Elfogadott végleges modell:** LiPo1S2P; soros cells=1, párhuzamos2 dokumentációs adat, nem kétszeres feszültség és nem új runtime architektúra.3200mV=0%,3700=50%,4200=100%; Rmin/Rmax a felhasználónál feszültség. Rt/Rb/tolerancia/maximális fizikai töltőfeszültség/plausibility/bekötés továbbra nem igazolt; ADC disabled.

**Folytatás:** modell és célzott trace frissítés; own MIT+külső licencek/origin, PUBLICATION_AUDIT és rövid README; tiszta aktuális-forrás normál build pristine pinnedNimBLE+trackedpatch, külön ADC TEST CONFIGURATION build; auditdöntés és source/bin/ELF mentés. Git-history/index és régi mentések nem változhatnak.

### 8/8 érdemi checkpoint — modell, licencek, tiszta normál build

A célzott ADC-check ASan/UBSan mellett PASS:3200→0%,3700→50%,4200→100%,
határok/monotonitás és hibás minták. Az ismeretlen ellenállás-tolerancia explicit
UNKNOWN; normal enabled=false. Nem módosult a keyboard/session/BLE runtime.

MIT standard szöveg és2023 copyright megőrzött,2026contributors hozzáadva;
Apache patch mindkét upstream fájljában módosítási értesítés,5 byteazonos
upstream LICENSE/NOTICE másolat. Új THIRD_PARTY_NOTICES és PUBLICATION_AUDIT.
CD rootMIT/LGPL3 fejléc eltérés és inaktív régi OTA-uploader eredete nyitott;
a korábbi történeti CD-MIT megjelölést a jelenlegi licenceltérés pontosítja.
Privát snapshot/bin/dump kizárás most a megosztott .gitignore-ban is szerepel.

A67 jelenlegi projektfájl és gitlink alatti CD forrás tiszta másolatából,
privát snapshot/generált könyvtár nélkül normál C3 build SUCCESS:518320flash,
25772 statikusRAM. Pristine pinnedNimBLE a helyi Git-objektumcache-ből,
ellenőrzött before hash-ekkel; a követett új patch alkalmazása sikeres.

**Pontos folytatás:** külön explicit ADC TEST CONFIGURATION build fut;
metadata/hash/offset, licencmásolat és végső sensitive/index/history/mentés
ellenőrzés; auditdöntés/commit-lista és új step-8 forrás–bin–ELF snapshot.
Nincs letöltés/frissítés vagy hardware/port/upload/commit/push.

### 8/8 lezárt eredmény —2026-10-07

**Modell:** felhasználói LiPo1S2P, cells=1; parallel2 dokumentációs adat.
3200mV=0%,3700mV=50%,4200mV=100%; nem cutoff/BMS/kapacitásmérés.
Rt/Rb/tolerancia/bekötés, maximális fizikai feszültség és alsó plausibility
nem megadott; normal disabled és hiányzó tolerancia UNKNOWN. ADC/BAS út
beköthető, nincs helyette automatikus tesztmérés. GPIO/pinout/partíció,
keyboard/state/session/debounce/security/runtime BLE változatlan.

**Publikálási audit:** PUBLICATION_AUDIT.md; jelenlegi67 forrásfájl,
7 elérhető commit/24 blob. Privátkulcs/credential találat nincs a célzott
mintakeresésben; ez nem teljes titokmentességi garancia. Történeti személyes
útvonalak helye dokumentált, értékük nem másolt az auditba; public cert nem
private key. P01 CD rootMIT/LGPL3 fejlécek eltérése; P02 inaktív ElegantOTA
uploader pontos licencverziója nem igazolt. Ezek nyitott scope-akadályok,
 nem téves hardverteszt-követelmények. Saját MIT kísérleti forrásként megosztható,
a teljes repo feltétel nélküli kiadását nem minősítjük késznek.

**MIT:** standard szöveg megőrzött,2023 bsvdoom copyright mellé2026contributors.
Nem átlicencelt SDK/CD/NimBLE vagy tisztázatlan régi fájl. THIRD_PARTY_NOTICES
és öt pontos upstream licenc/NOTICE másolat; Apache patch mindkét módosított
fájlban jelentős módosítási értesítés. read-guard végrehajtási logika változatlan,
metaadat afterhash-ek frissültek. Régi7/B/7ADC patch/binary mentések megmaradtak.

**Ellenőrzések:** egy célzott ADC-check PASS ASan/UBSan mellett (LSan kikapcsolt),
3200/3700/4200, határok/clamp/monotonitás/invalid minta és filter/timestamp/wrap.
Normál67 fájlos tiszta másolatból egy SUCCESS C3 build, .git/.pio/private
snapshot/kézi librarypótlás nélkül; pinned upstream NimBLE pristine beforehash
után a követett patch reprodukálódott. Külön ADC TEST CONFIGURATION egyszeri
SUCCESS C3 build, hipotetikus100k/100k és1%/4250maximum/2800plausibility,
ugyanazzal a felhasználói3200/4200 modellel; ez nem mérési eredmény.

| Build | Flash / statikus RAM byte | Delta lezárt ADC518328 /25772 | Delta7/B518304 /25772 |
| --- | --- | --- | --- |
|normal|518320 /25772|-8 /+0|+16 /+0|
|adc-test|523724 /26060|+5396 /+288|+5420 /+288|

Normál8,26s, ADC7,48s; mindkét exit0/SUCCESS, hat korábbi upstream warning/build,
saját forráshiba nincs. Nem futott változatlan teljes regressziós sor, távoli CI
vagy Docker. Workflow statikus ellenőrzése megfelel: recursive submodule,
pinnedCore/scriptkövetelmények; ebben a8/8 lépésben nem módosult.

**Pontos csomagok:** Core6.2.0; espressif32@6.13.0;
Arduino2.0.17 /IDF4.4.7, framework3.20017.241212+sha.dcc1105b;
RISC-V8.4.0+2021r2-patch5; esptool2.41100.0 /4.11.0;
SCons4.41101.0 /4.11.1; NimBLE2.5.1@5ae2c43f62c5a588a25c1de68f7d17b6f3b75adb
+követett read-guard; CD1.0.2 gitlinkaa6959c25a9ad7160fd515f73d8727aa86fc1ffd.
Nincs csomagletöltés/frissítés. A pontos parancsok az audit végső ellenőrzésében.

**normal**: `no-git-unknown-src7a32e9a39cd5`, UTC `2026-10-07T21:00:30Z`; bin 544640byte SHA256 `9d4dfadee8cfbab7888a7eec814cd0a91510ce41f50ec2a651d7279f788e41b1`; ELF SHA256 `27a27869d64d1e259a4dc63580384d78cc721ad6f91b58ea4d6e24b0983bdefd`.

**adc-test**: `no-git-unknown-src2d06451e9e9d-ADC-TEST-CONFIGURATION`, UTC `2026-10-07T21:00:51Z`; bin 550560byte SHA256 `fe5c7b1aee41ea5541cb3b1bfed695d7737d735a7008db4a4e8cb5cc6a5807c7`; ELF SHA256 `4855b6ea7f2b3bffa36ad26af3c4c9d549131f6c2212cb467f4e1d1b2095ecb4`.

Új patch SHA256 `1d2208a1a01222eab0a0252afd86ccac409f0f14c4893a6d07a84b05f67970ed`.

A normál no-git-unknown az izolált másolat valós státusza; runtime fingerprint
byteazonos a dirty root forrással, eredeti HEAD/staged/untracked állapot külön
origin/preservation sidecarban. A TEST image fingerprint/makró/JSON külön jelölt.
Az azonosító/UTC nem tesz ígéretet bitazonos újrafordításra vagy hostvételre.

Artifact-plan mindkét image-hez ellenőrzött: bootloader0000, partitions8000,
boot_app0D000, app10000, határok/erase/SHA és binbe ágyazott ELF hash egyezik.
A bootloader/partitions/initializer byteazonos az előző mentéssel.
A7/A feltételes első-írási terv változatlan; régi layout/NVS/bond bizonytalan,
régi teljes flashmentés nincs, hardware és first-boot adatmegőrzési előfeltételek
fennállnak. Nem indult upload/port/reset/flash/NVS/bond/eFuse művelet.

**8/8 módosított fájlok:** .gitignore, LICENSE, README.md, PROGRESS.md,
include/BatteryAdcConfig.h, scripts/nimble-read-guard.patch és .json,
tools/adc_test_configuration.h, tools/check_adc_battery.cpp.
**Új:** PUBLICATION_AUDIT.md, THIRD_PARTY_NOTICES.md,
licenses/NimBLE-Arduino/LICENSE és NOTICE, src/nimble/LICENSE és NOTICE,
src/nimble/ext/tinycrypt/LICENSE ugyanezen licenses gyökér alatt.
AUDIT és az összes korábbi runtime/build/CI változtatás megőrzött.

**Későbbi commit teljesség:** az audit explicit untracked-listája minden új
include/src/scripts/tools/requirements/docs/licenses fájlt felsorol. Előző
tracked változások .github/workflows/platformio.yml, .gitmodules,
platformio.ini, src/main.cpp, bin/bbread.py és recv.sh, docker-compose.yml,
docker/Dockerfile, inaktív platformio_upload.py; a két staged HUN/ElegantOTA
gitlinktörlés és változatlan rögzített CD gitlink is megőrzött. Ezeket nem
stage-eltük/commitoltuk. **Kizárandó:** .local-snapshots, .pio, private/hardware
backups, artifacts/bin/ELF/dump/log/sidecar, secret és csomagcache. A .gitignore
projektszintű kizárása helyi info/exclude nélkül is érvényes; követett fájlt
vagy historyt nem tisztít meg, licenc/privacyscope később külön rendezendő.

**Végső tartós mentés:** `.local-snapshots/step-8-build-20261007/`:
normál teljes forrásarchive/manifest és bin/ELF/header/build-info/extraimage/
artifact-plan/log, külön adc-test/ forrás és TEST CONFIGURATION artifactpár;
SHA256SUMS, native/build/provenance/scan/preservation eredmények. Ez új
rekonstruált build, nem az eredeti eszközfirmware vagy hardverflashdump.

**Elhalasztott hardveres sorrend:** ellenőrzött adapter/bootmód/flashkapacitás,
két egyező teljes régi flashmentés; oldlayout/otadata/NVS/bond és first-boot
review; Rt/Rb/tolerancia/fizikai max/ADC-bekötés és multiméteres kalibráció;
külön engedélyezett első írás után115200boot/heartbeat és bin–ELF–ID napló;
magyar gépelés/modifierek/hatgombos működés/reconnect/sleep-wake/gyors gépelés,
Windows/Linux/Android eredmény külön, battery GATT/UI; debounce/ghosting,
táp/RF és tünet alapján tartós stabilitás. **Minden hardveres elfogadás NOT TESTED.**

**Folytatási pont:** offline8/8 kész. A teljes-repo publikálási P01/P02 scope,
P03 jelenlegi fájlok commitba kerülése és P2 privacy/cert/eszközazonosítás
review a PUBLICATION_AUDIT végső döntése szerint; nincs automatikus következő
művelet. Hardveres elfogadás elhalasztott, jelenlegi mentések és7/A előfeltételek
az irányadók. Commit/push/publikálás vagy új fejlesztési lépés nem történt.

### Végső offline lezárás —2026-10-08

**Elkészült:** az aktuális README, PUBLICATION_AUDIT és lezárt8/8 eredmény
alapján célzott dokumentáció/configuration/runtime egyezésellenőrzés; az audit
nem indult újra. A README most a korábbi végső buildméreteket és native PASS
eredményt dátummal közli, egyértelműen ismeretlennek jelöli a régi fagyás okát
és megszűnését, a Windows/Linux/Android működést célként, nem eredményként.
Az1S2P modell,3200/3700/4200 és kerekített lineáris becslés, ADC disabled,
hiányzó hardverparaméterek és ellenőrizetlen Battery UI összhangban van a kóddal.
Működési/buildváltoztatás nem indokolt; csak README.md, PUBLICATION_AUDIT.md és
PROGRESS.md módosult. Licenc/NOTICE, függőség, pinout és összes korábbi forrás
és mentés megőrzött; nincs általános refaktor vagy új funkció.

**Felelős és nyitott állapot:** a felhasználó külön kezeli a CD74HC4067
MIT/LGPL-eltérését (P01) és az új, tiszta repóba publikálást, beleértve az
inaktív régi OTA-segéd eredetét/sorsát (P02). Ezek nem megoldottak. A P03
forráslista az új repó teljes aktív implementációjának és rögzített patch/
függőségek/licencek átadásához támpont; nem írja elő a régi Git-history vagy
privát/tisztázatlan történeti fájlok átvételét. A saját MIT és a külső licencek
elkülönítése változatlan, bináris terjeszthetőségi nyilatkozat nincs.

**Ellenőrzések:** statikus összevetés a platformio.ini, KeyboardPins/Config,
KeyboardState/Session/Hid, BatteryAdcConfig/Math/Source, BatteryState,
Diagnostics és buildhookok alapján; normál konfigurációban nincs ADC/test
makró, enabled=false, egyetlen aktív HID-megoldás és pontos függőségek.
A bemeneti67 forrásfájl byteazonos a step-8 source-manifesttel; a korábbi
normál518320/25772 és ADC TEST CONFIGURATION523724/26060 build eredménye
2026-10-07-i, nem mai teszt. Dokumentációs módosítás miatt új build/native/
runtime-teszt, távoli CI/Docker, teljes licencek-/titokaudit nem futott.

**Ellenőrizetlen:** minden hardveres elfogadás továbbra NOT TESTED: tényleges
flashváltozat/bekötés, régi flashmentés/layout/otadata/NVS/bond/első boot,
hostpárosítás/reconnect/magyar gépelés/gyors nyomások/alvás-ébredés,
ADC pontosság/kalibráció és Battery GATT/UI, debounce/ghosting, táp/RF/hatótáv,
tartós stabilitás. A korábbi fagyásra nincs bizonyított ok vagy megszűnés.

**Pontos folytatási pont:** README „Későbbi hardveres ellenőrzés sorrendje”
és a jelen dokumentum lezárt7/A terve. Kezdés a kizárólag eszközön létező
régi teljes flash két egyező privát mentésével; utána régi layout/NVS/bond
és first-boot adatmegőrzési/visszaállítási review, fizikai paraméterek, majd
külön későbbi feltöltési utasítás szerinti indulás és hostonkénti ellenőrzés.
A firmware/ELF/build-ID, artifactterv és SHA-k változatlan hiteles alapja
`.local-snapshots/step-8-build-20261007/`, külön normal és ADC TEST CONFIGURATION
párral. A mostani lezárás dokumentációs, nem új firmware vagy flashmentés.
Nincs portművelet, firmware-feltöltés, Git-előzmény-módosítás, új repó,
commit/push vagy publikálás. Az offline lezárás kész; nincs automatikus továbblépés.

**Lezárási ellenőrzés:** csak a fenti három dokumentum változott; minden
runtime/config/build/licencfájl byteazonos a bemenettel. A222 korábbi mentett
fájl SHA256-ja, Git-index és history változatlan; a történeti PROGRESS-törzs
megmaradt, helyi dokumentumlinkek és diff/cached-diff whitespace ellenőrzése
PASS, a referenciarepo-k és a CD submodule tiszták. Build/runtime-teszt nem
ismétlődött. A végső dokumentáció a munkafában van; a step-8 mentés a korábbi,
2026-10-07-i hiteles buildet és akkori dokumentumállapotot őrzi.
