# Publikálási audit —8/8 offline

**Aktuális lezárás: 2026-10-08 — offline munka kész.** A felhasználó külön
kezeli a CD74HC4067 licenceltérését és az új, tiszta repóba publikálást,
beleértve a régi OTA-segéd kérdését. P01/P02 továbbra nyitott, nem megoldott.
A publikálási forráslista átadási támpont; itt nincs repólétrehozás,
history-módosítás vagy publikálás. A hardveres elfogadás elhalasztott.

Dátum:2026-10-07. Vizsgált cél: a jelenlegi munkafa saját forrása és a
helyileg elérhető7 commit /24 blob (`git rev-list --objects --all`), HEAD
`54adbb04939d55e4c1cbb485fd5fb923392e7729`. A staged két régi gitlinktörlés,
a korábbi unstaged/untracked implementáció megőrzött. Az index/history nem módosult.
Nincs commit/push, új remote repository, release vagy tényleges publikálás.

Az eredeti AUDIT és az alábbi 2026-10-07-i vizsgálati eredmények történetiek;
az aktuális PROGRESS első sora és a jelen dokumentum végső lezárása az irányadó.
Nincs hardware/port hozzáférés.
A vizsgálat nem teljes jogi vélemény vagy minden titkot felismerő ellenőrzés,
és nem terjed ki nem elérhető remote refs/reflog/unreachable objektumokra.

## Publikálást akadályozó konkrét kérdések

| ID/prioritás | Hely/bizonyíték | Hatás és szükséges intézkedés |
| --- | --- | --- |
| P01 /P1, nyitott külső licencscope | `lib/CD74HC4067/LICENSE:1`, `lib/CD74HC4067/src/CD74HC4067.{h,cpp}:4` a gitlinkaa6959c… alatt | root MIT és LGPL3.0 fejléc ellentmond. Nem állítunk kizárólag MIT-függőséget vagy teljesen tiszta binary licenceket; upstreamtől/file-eredetből tisztázandó vagy konzervatív LGPL-terjesztési feltételek külön review-zandók. **A felhasználó kezeli, továbbra nyitott.** A submodule változatlan, nem átlicencelt. |
| P02 /P1, nyitott történeti fájlscope | `platformio_upload.py:5`, HEAD blob`b22e8d5da12d5ce89d77ad6e3879b35291268185`, bevezetés`b535cadce7de00cb7c6c63b91eb44d62be76a321` | ElegantOTA-ra utaló inaktív átvett segéd, saját licencfejléc nincs. Cache3.1.7 uploader hasonló, nem azonos; AGPL licence nem igazolja a régi példányt. Pontos eredeti verzió/licenc vagy külön publikálási kivezetés kell. **A felhasználó az új, tiszta repó publikálásával együtt kezeli, továbbra nyitott.** Nem adunk neki új MIT-engedélyt, nem töröltük; history is megőrzött. |
| P03 /P1, publikálási teljesség | `git status --short`: új src/include/scripts/tools/docs/requirements/license fájlok többsége még untracked; régi HEAD nem ezt építi | A felhasználó új repójába a végleges munkafa szükséges aktív forrásai, konfigurációja, buildhookjai, patch/metaadatai, függőségrögzítései és licencértesítései is átadandók. Csak HEAD klónozása hiányos. A történeti/OTA/privát fájlscope külön választandó; most nincs staging/commit. |

A forrás megosztásának akadálya **nem** önmagában a hardverteszt hiánya.
P01/P02 scope-ja pontosan elhatárolható a saját MIT-kódtól; ezek feloldása vagy
külön tudatos publikálási scope szükséges a teljes repo feltétel nélküli kiadásához.
Nem választottunk helyettük új libraryt vagy újratervezést ebben a lépésben.

## Elvégzett célzott rendezés

- `.gitignore`: a helyi exclude mellé a projekttel megosztott
  `/.local-snapshots/`, backup/artifacts könyvtárak, bin/dump/log-sidecar kizárás.
  A184 korábbi mentett fájl nem törölt vagy átírt. Az indexben nincs snapshot/dump/bin.
  Már követett fájlt az ignore nem töröl; a történeti cert és uploader továbbra követett.
- `LICENSE:3–4`: eredeti2023 bsvdoom MIT értesítés megtartva, új saját
  2026contributors copyright hozzáadva; standard feltételszöveg változatlan.
  [MIT elsődleges szöveg](https://opensource.org/license/mit).
- `THIRD_PARTY_NOTICES.md`: explicit own MIT / Apache patch / SDK / CD /
  tisztázatlan történeti scope; nem teljes firmware-licencminősítés.
- `licenses/NimBLE-Arduino/`: pinned LICENSE/NOTICE és vendorizált licencek
  byteazonos másolata. A patch mindkét upstream módosított fájlhoz jelentős
  módosítási értesítést ad; működésváltozás nincs. Apache§4 forrása az ottani LICENSE.
- `include/BatteryAdcConfig.h`:1S2P soros cells=1,3200/4200 modell; párhuzamos2
  csak dokumentáció, nem feszültségszorzó. Backend false; Rt/Rb/max/plausibility
  hiányzó és ellenállás-tolerancia explicit UNKNOWN. Nem kevertünk modelldatát
  fizikai maxfeszültséggel vagy akkuvédelmi policyval.

## Titkok, tanúsítványok és privát adatok

A jelenlegi Git-kandidátum és minden helyileg elérhető történeti blob UTF-8/bytes
mintakeresése privát PEM-kulcs, credential-assignment/tokenalak és személyes útvonal
kategóriákra történt. **Privát kulcsot/credentialt nem találtunk**, de ez nem
általános titokmentességi garancia. Értékeket nem másolunk az auditba.

| Hely | Kategória / prioritás | Eredmény/intézkedés |
| --- | --- | --- |
| `src/certs.h:10,21,51`, blob`aaaea7fc4ba8b40d7502044038a7e3bfa94e09da` | nyilvános tanúsítvány/PUBLIC KEY, P2 provenance | nem privát kulcs és nem credential; a BLE-buildben nincs include. Nem minősítettük automatikusan titoknak, nem töröltük. Nem saját MIT; történeti eredet/terjesztési scope review marad. |
| `AUDIT.md:42,52,61`, `PROGRESS.md:85` | helyi személyes munkakönyvtár-útvonal, P2 | történeti dokumentumok megőrzöttek; publikálás előtt tulajdonosi privacy-review és szükség esetén külön redaktált publikálási dokumentummásolat. A konkrét útvonalérték itt nem szerepel. |
| `docker/Dockerfile:7,8`, blob`8a6d95e2734dac1e9bba74e642b05ab8b91a51c7` | konténerbeli service-account útvonal, P3 | nem személyes hostútvonal, nem credential; false positive. |
| `.local-snapshots/`, generált .pio/bin/ELF/log/artifact manifestek | privát backup és generált gépútvonalak, P1 megelőzés | projektszintű ignore; a saját forrásfájlok explicit kiválasztásával készül a tiszta másolat. Mentések/uploadmanifestek nem kerülnek publikált forrásba. |
| `src/secret.h` | nincs jelenlegi fájl vagy elérhető történeti blob | a build nem igényli; ignore megmarad. Nem történt credential-rotáció vagy history rewrite. |

A régi history csak külső HUN/ElegantOTA gitlinkeket tartalmaz, azok kódját nem
vendorizálja ezekben a24 blobban. Más távoli fork/klón nem vizsgált. Ha később
valódi titok derül ki, külön célzott intézkedés szükséges; az ignore nem tisztítja
meg a historyt. Most semmit nem írtunk át vagy rotáltunk.
[GitHub érzékeny history kezelése](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository).

## Reprodukció és build/artifact scope

- `requirements-build.txt:2`, `platformio.ini:38,48–59`: Core6.2.0; platform6.13.0;
  Arduino2.0.17/IDF4.4.7 csomag3.20017.241212+sha.dcc1105b; RISC-V8.4.0+2021r2-patch5;
  esptool2.41100.0/4.11.0; SCons4.41101.0/4.11.1. NimBLE pontos5ae2c43… és CD
  gitlinkaa6959c… az egyetlen aktív két library. Nincs OTA/web/Wi-Fi dependency.
  Pinned főcsomagok nem jelentik a Python összes tranzitív csomagjának lockját,
  és a UTC/forrásstátusz miatt nem ígérünk bitazonos fordítást.
  [PIO verzió/commit forrásrögzítés](https://docs.platformio.org/en/latest/librarymanager/dependencies.html).
- `scripts/prepare_build.py:17–31`, `nimble-read-guard.{patch,json}`: exactHEAD,
  before/after hash és git apply --check; váratlan módosításnál fail-closed.
  Tiszta upstreamre újra alkalmazva ellenőrzött, eredmény alább; régi patchelt
  generált könyvtár vagy privát snapshot nem a reprodukció forrása.
- `scripts/prepare_flash_plan.py:37–43,55–83` és custom.csv: a boot_app0
  alapE000 helyettD000, mind FLASH_EXTRA_IMAGES és UPLOADERFLAGS korrigált;
  méret/SHA/range/erase-szektor/partíció/binbe ágyazott ELF hash ellenőrzés.
  Ez artifactterv, nem régi flashlayout/migráció és nem feltöltési jóváhagyás.
- `tools/adc_test_configuration.h` és `BatteryAdcConfig.h`: normál disabled,
  külön ADC TEST CONFIGURATION hipotetikus osztó/max/plausibility; aktív teszt
  header a fingerprintben, metadata true. Korábbi42%-os synthetic fixture
  külön makróval megőrzött, normál/ADC tesztben nem aktív. Együtt tiltott.
- `.github/workflows/platformio.yml:9–18`: recursive submodule checkout,
  Python3.10, pinnedCore, pio esp32c3; megfelel a jelenlegi hook/submodule/build
  igénynek. Nincs upload/release. Nem indokolt workflow-módosítás. Actions v3/v4
  főverzió-tag nem immutable SHA, P2 későbbi CI supply-chain rögzítés mérlegelendő;
  ez nem a firmware dependency pin feloldása. **Távoli CI/Docker NOT RUN**.

## Dokumentált kísérleti korlátok

P2: `src/KeyboardHid.cpp:62` örökölt PnP értékek, nincs saját vendor-hozzárendelés
bizonyíték. Kísérleti forrásban feltüntetve, kereskedelmi/végleges eszközazonosítás
előtt külön rendezendő; most nincs HID-descriptor/security/API változtatás.
P2: handle-only notify verseny, lokális eredmény nem ACK; FIFO overflowkor köztes
nyomások veszhetnek; BAS nélküli profil teljes HOGP-minősítése nem igazolt.
Ezek a README/PROGRESS-ben dokumentáltak, nem a régi fagyás okának bizonyítékai.

**Hardveres későbbi ellenőrzések:** Windows/Linux/Android hoston külön pairing,
magyar gépelés/8 modifier/hatgombos működés; reconnect held-release szabály,
readiness/unsubscribe/sleep-wake; gyors press/release; ADC0 fizikai hozzáférés,
Rt/Rb/tolerancia/max és multiméteres kalibráció; battery UI; debounce/ghosting;
táp/RF és tünet alapján tartós stabilitás. Mind NOT TESTED. Előtte a7/A szerinti
két egyező teljes régi flashmentés és oldlayout/NVS/bond/elsőboot review szükséges.
Ezek nem helyettesíthetők sikeres builddel, de nem önmagukban forráspublikálási tilalmak.

## Tényleges végső ellenőrzések

Célzott ADC-check **PASS**, ASan/UBSan:3200→0,3700→50,4200→100;
3199/4250 clamping csak az explicit teszt-plausibility tartományában,
2799/4251 érvénytelen,2800…4250 monoton;0/hibás osztó/range/raw rail,
invalid burst és tényleges mérési timestamp ellenőrzött.
`tools/check_adc_battery.cpp:41–100` a firmware `src/BatteryAdcMath.cpp`
kódját használja, konkrét elvárt értékekkel. LeakSanitizer a korábbi sandbox
ptrace-korlát miatt kikapcsolt. Változatlan teljes regressziós sor nem futott.

Tiszta **67 fájlos** normál projektmásolat: valamennyi jelenlegi követett és
untracked projektfájl, CD gitlink alatti hét forrás/licencfájl; nincs .git/.pio,
privát snapshot, secret vagy régi generált library. A helyi csomagcache használható
volt; NimBLE új checkout a pinned Git-objektumokból, előtte két pristine hash,
utána két patchelt hash egyező. Az upstream öt licenc/NOTICE másolata byteazonos.
Az új patch csak a módosítási értesítéseket egészíti ki, nem a read-guard viselkedését.

```sh
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iinclude tools/check_adc_battery.cpp src/BatteryAdcMath.cpp -o /tmp/keyboard-step8-20261007/check-adc
ASAN_OPTIONS=detect_leaks=0 /tmp/keyboard-step8-20261007/check-adc
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step8-20261007/build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step8-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -d /tmp/keyboard-step8-20261007/normal-project -e esp32c3
PLATFORMIO_CORE_DIR=/tmp/keyboard-step3-20261007/core PLATFORMIO_BUILD_DIR=/tmp/keyboard-step8-20261007/adc-test-build PLATFORMIO_LIBDEPS_DIR=/tmp/keyboard-step8-20261007/libdeps /tmp/keyboard-step1-20261006/venv/bin/pio run -d /tmp/keyboard-step8-20261007/adc-test-project -e esp32c3
```

| Build | Flash / statikus RAM byte | Delta lezárt ADC-kiegészítés normál518328 /25772 | Eredmény |
| --- | --- | --- | --- |
| Normál, ADC disabled |518320 /25772|-8 /+0|SUCCESS,exit0;8,26s|
| ADC TEST CONFIGURATION |523724 /26060|+5396 /+288|SUCCESS,exit0;7,48s|

A7/B normál518304 /25772 baseline-hoz a végső normál delta+16 /0,
a teszt+5420 /+288. Flash limit1027200, statikus RAM limit327680;
az image fájlmérete nem ugyanaz a PIO flashfogyasztás. Mindkét buildben hat
korábbi upstream NimBLE warning, saját módosított forrás warningja nincs.
Letöltés/frissítés nem történt. Az eltérő buildazonosítóval/konfigurációval
fordított méretkülönbség nem működési vagy stabilitási bizonyíték.

A normál ID **no-git-unknown**, mert a tiszta másolat nem hordozott .git-et;
ez nem tiszta HEAD-ként álcázza a módosított forrást. A runtime fingerprint
byteellenőrzötten megegyezik az aktuális root forrással. A mentés külön rögzíti
az eredeti HEAD-et és dirty/staged/untracked állapotot. A teszt saját INI-je
csak a KEYBOARD_ADC_TEST_CONFIGURATION makróban tér el; sem synthetic42%,
sem tesztbackend nem került normál konfigurációba.

**normal**: `no-git-unknown-src7a32e9a39cd5`, UTC `2026-10-07T21:00:30Z`; bin 544640byte, SHA256 `9d4dfadee8cfbab7888a7eec814cd0a91510ce41f50ec2a651d7279f788e41b1`; ELF SHA256 `27a27869d64d1e259a4dc63580384d78cc721ad6f91b58ea4d6e24b0983bdefd`.

**adc-test**: `no-git-unknown-src2d06451e9e9d-ADC-TEST-CONFIGURATION`, UTC `2026-10-07T21:00:51Z`; bin 550560byte, SHA256 `fe5c7b1aee41ea5541cb3b1bfed695d7737d735a7008db4a4e8cb5cc6a5807c7`; ELF SHA256 `4855b6ea7f2b3bffa36ad26af3c4c9d549131f6c2212cb467f4e1d1b2095ecb4`.

Patch SHA256 `1d2208a1a01222eab0a0252afd86ccac409f0f14c4893a6d07a84b05f67970ed`.

Mindkét artifact-plan:0000 bootloader,8000 partitions,D000 boot_app0,
10000 firmware; méret/hash/partícióhatár és törlési szektorok nem fednek át,
firmware belefér app0-ba, binbe ágyazott ELF SHA egyezik. A három extraimage
byteazonos az előző lezárt mentéssel. App0 inicializálási terv változatlan;
régi hardverlayout/otadata/NVS/bond nem ismert, feltöltés nincs.
Forrás–bin–ELF és tesztpár: `.local-snapshots/step-8-build-20261007/`,
privát, megosztott .gitignore által kizárt, SHA256SUMS+forrásmanifestekkel.

## Publikálási forráslista a felhasználó új repójához

A régi HEAD önmagában nem az ellenőrzött megoldás. Az alábbi új fájlok szükségesek;
a két dokumentált licencscope rendezése nélkül a lista nem publikálási engedély.
A lista az ellenőrzött munkafa teljességét rögzíti, nem írja elő a régi
Git-történet vagy a vitatott történeti fájlok átvételét. Ezekről és a személyes
útvonalat tartalmazó auditnaplók publikálási változatáról a felhasználó dönt.
A submodule forrásfájlokat nem kell a root repóba vendorizálni: a CD gitlink
`aa6959c25a9ad7160fd515f73d8727aa86fc1ffd` és .gitmodules együtt szükséges.

```text
AUDIT.md
PROGRESS.md
PUBLICATION_AUDIT.md
THIRD_PARTY_NOTICES.md
include/BatteryAdcConfig.h
include/BatteryAdcMath.h
include/BatteryAdcSource.h
include/BatteryState.h
include/Diagnostics.h
include/KeyboardConfig.h
include/KeyboardHid.h
include/KeyboardLayout.h
include/KeyboardLedValue.h
include/KeyboardLink.h
include/KeyboardPins.h
include/KeyboardReport.h
include/KeyboardSession.h
include/KeyboardState.h
licenses/NimBLE-Arduino/LICENSE
licenses/NimBLE-Arduino/NOTICE
licenses/NimBLE-Arduino/src/nimble/LICENSE
licenses/NimBLE-Arduino/src/nimble/NOTICE
licenses/NimBLE-Arduino/src/nimble/ext/tinycrypt/LICENSE
requirements-build.txt
scripts/nimble-read-guard.json
scripts/nimble-read-guard.patch
scripts/prepare_build.py
scripts/prepare_flash_plan.py
src/BatteryAdcMath.cpp
src/BatteryAdcSource.cpp
src/BatteryState.cpp
src/Diagnostics.cpp
src/KeyboardHid.cpp
src/KeyboardSession.cpp
src/KeyboardState.cpp
tools/adc_test_configuration.h
tools/battery_compile_fixture.h
tools/check_adc_battery.cpp
tools/check_battery.cpp
tools/check_hid.cpp
tools/check_led.cpp
tools/check_session.cpp
tools/check_state.cpp
tools/step2_hid_expected.h
```

A meglévő módosított fájlok: `.gitignore`, `.gitmodules`, `LICENSE`, `README.md`,
`platformio.ini`, `src/main.cpp`, `.github/workflows/platformio.yml`,
`bin/bbread.py`, `bin/recv.sh`, `docker-compose.yml`, `docker/Dockerfile`,
`platformio_upload.py`; a staged régi HUN/ElegantOTA gitlinktörlések is a
korábbi elfogadott állapot részei. Az utóbbi OTA-helper további kezelése P02.
Az indexet nem módosítottuk. `custom.csv` és további változatlan követett
fájlok továbbra szükségesek, az izolált67 fájlos manifest tartalmazza őket.

**Kizárandó:** `.local-snapshots/`, `.pio/`, hardware/private-backups,
generált artifacts, dump/bin/ELF/log/sidecar, secret és gépi cache.
Ezekből sem forrásklón, sem automatikus bináris release nem készült.
.git/info/exclude nélkül is a megosztott `.gitignore:45–51` adja a vizsgált
snapshot/bin/dump/artifact kizárást. A184 korábbi mentett fájl és Git-index
byteazonos, történeti PROGRESS teljes törzse és referenciaprojektek megőrzöttek.

## Konkrét publikálási döntés

**A saját MIT-implementáció kísérleti, hardveren nem validált forrásként
megosztható a dokumentált korlátokkal. A teljes jelenlegi repo és történeti
fájlscope feltétel nélküli publikálását nem minősítjük késznek:** P01 CD
MIT/LGPL eltérés és P02 régi uploader eredet/licenc tisztázandó; P03 az új
fájlok hiánytalan átadása szükséges a felhasználó új repójába. P2 privacy-/cert-provenance és
végleges eszközazonosítás külön review. A saját MIT nem oldja fel a külső
licenckérdéseket, és a firmware-bináris terjesztési review nincs lezárva.

**8/8 offline fejlesztés, dokumentáció és audit kész**, pontos fennmaradó
publikálási kérdésekkel. A hardveres elfogadás elhalasztott és nincs kipipálva;
a hiánya önmagában nem tiltja a saját kísérleti forrás megosztását.
Nincs portművelet, flashírás/olvasás, commit/push vagy publikálás.

## Végső offline lezárás és folytatási ellenőrzőpont —2026-10-08

**Offline lezárva, az audit nem indult újra.** A jelenlegi README/PROGRESS
állításait összevetettük a platformio.ini, BatteryAdcConfig/Math/Source,
KeyboardPins/Config/State/Session, KeyboardHid/BatteryState és diagnosztika,
illetve a követett buildhookok tényleges működésével. Működési vagy
buildmódosítás nem volt indokolt; csak a három lezáró dokumentum változott.
P01/P02 felelőse a felhasználó, állapotuk nyitott; a teljes repó és a bináris
terjesztés feltétel nélküli megfelelőségét továbbra nem állítjuk.

A korábbi PASS trace-ek és két SUCCESS C3 build eredménye érvényes történeti
ellenőrzés; új build/runtime-teszt, távoli CI/Docker vagy licenc-/titokaudit
nem futott. A régi fagyás oka és megszűnése nem bizonyított, hostkompatibilitás
csak cél. ADC default disabled, 1S2P/3200–4200 lineáris modell; valódi
Rt/Rb/tolerancia/bekötés/fizikai maximum/plausibility nincs megadva, a
multiméteres kalibráció és az eFuse TP kalibráció elérhetősége nem ellenőrzött;
Battery UI és minden korábbi hardveres elfogadás NOT TESTED.

**Későbbi hardveres folytatás:** a README „Későbbi hardveres ellenőrzés
sorrendje” listája és a PROGRESS lezárt7/A feltételes artifact-/első-írási és
visszaállítási terve. Első írás előtt két egyező teljes régi flashmentés,
layout/otadata/NVS/bond és első-boot adatmegőrzési review szükséges.
A hiteles korábbi normál és ADC TEST CONFIGURATION forrás–bin–ELF párok
változatlanul a `.local-snapshots/step-8-build-20261007/` alatt vannak;
a mostani dokumentációs lezárás nem új firmwarebuild és nem flashmentés.
Nincs portnyitás, feltöltés, új repó, Git-előzmény-módosítás, commit/push vagy publikálás.

Lezárási egyezésellenőrzés: csak README/PROGRESS/PUBLICATION_AUDIT változott;
runtime/config/build/licencfájlok és222 korábbi mentett fájl byteazonosak,
index/history és történeti PROGRESS megőrzött. Helyi dokumentumlinkek és
diff/cached-diff whitespace PASS; referenciák és CD submodule változatlanok.
