# AGENTS.md — OSM Scout for Sailfish OS

Offline maps & navigation app for Sailfish OS. Native Qt5/QML frontend on top of the
bundled **libosmscout** C++ library (`dependencies/libosmscout/`, which has its own
detailed `AGENTS.md`). C++20, Qt 5.6+, `harbour-osmscout` binary.

## Big Picture

- **`src/*.cpp/.h`** — thin C++ layer exposing `QObject` models to QML. The actual
  map DB, rendering, routing, and location lookup live in `libosmscout` /
  `libosmscout-client-qt` (linked as `OSMScout`, `OSMScoutMap`, `OSMScoutMapQt`,
  `OSMScoutClientQt`, `OSMScoutGPX`). Don't reimplement mapping logic here — extend
  the library or use its client API (`osmscoutclientqt/OSMScoutQt.h`).
- **`src/OSMScout.cpp`** — `main()`. Central place where the app is wired:
  `OSMScoutQt::NewInstance()...Init()` builder configures tile providers, style
  sheets, icon dirs, cache, and custom POI types; then QML types are registered.
- **`qml/`** — UI. `main.qml` (device, `Sailfish.Silica`), `desktop.qml`
  (`--desktop` mode, plain Qt Quick, no Silica needed for local dev). Reusable
  components in `qml/custom/`, screens in `qml/pages/`.
- **Data flow**: OSM databases downloaded per-region into `Downloads/Maps` or SD
  `Maps` dirs → libosmscout DB → client-qt query/render → QML MapComponent.
  User waypoints/tracks stored via `Storage` (SQLite) as Collections, exportable to GPX.

## Key Conventions (specific to this repo)

- **Exposing C++ to QML**: every new model/type must be (1) added to `SOURCE_FILES`/
  `HEADER_FILES` in the root `CMakeLists.txt`, and (2) registered in `OSMScout.cpp`
  via `qmlRegisterType<T>("harbour.osmscout.map", 1, 0, "Name")`. Non-QObject value
  types passed through signals also need `qRegisterMetaType<T>("T")` there.
- **`Storage`** uses a process-wide singleton: `Storage::initInstance(dataDir)` /
  `Storage::getInstance()` / `Storage::clearInstance()` (lives on its own thread).
- **Adding a QML file**: also list it in `QML_FILES` in `CMakeLists.txt` (keeps
  QtCreator + install happy). Reuse `qml/custom/` widgets; follow existing page style.
- **Assertions stay ON** in Release/RelWithDebInfo (`-DNDEBUG` is deliberately
  stripped in `CMakeLists.txt`). Don't rely on asserts being compiled out.
- **Extra headers**: generated `harbour-osmscout/private/{Config,Version}.h` come from
  `CMakeMod/*.h.cmake` — reference build-time defines like `OSMSCOUT_SAILFISH_VERSION_STRING`.

## Translations

- User-facing strings use `tr()` (C++) / `qsTr()` (QML). Add a language by adding a
  `translations/<lang>.ts` entry to `TRANSLATION_TS_FILES` in `CMakeLists.txt`; the
  build runs `lupdate`/`lrelease` automatically to produce `.qm` files.

## Map Styles

- `stylesheets/*.oss` (style rules) + `stylesheets/map.ost` (type defs). CTest target
  `CheckStyleSheet-*` validates each `.oss` against `map.ost` with `OSTAndOSSCheck`
  (`--warning-as-error`). Custom `_waypoint_*` / `_track` POI types must match the set
  registered in both `OSMScout.cpp` and the CTest args in `CMakeLists.txt`.

## Build / Run / Debug

- **Local desktop dev** (fastest iteration, needs `libsailfishapp` installed locally):
  ```bash
  cmake -DCMAKE_BUILD_TYPE=Debug -DQT_QML_DEBUG=yes -DOSMSCOUT_ENABLE_SSE=true ..
  make -j$(nproc) && ./harbour-osmscout --desktop
  ```
- **Sailfish device build** (RPM via SDK): `sfdk build --enable-debug` then
  `sfdk deploy --sdk --debug`. Spec: `rpm/harbour-osmscout.spec`.
- **Sanitizers**: `-DSANITIZER=address|undefined|thread`.
- **Tests**: `ctest --output-on-failure` (mainly stylesheet checks + perf binaries
  `PerformanceTest`, `SearchPerfTest`, `Routing`, `MultiDBRouting`).
- Debugging device crashes (coredump/gdbserver + sshfs sysroot): see `wiki/Debugging.md`.

## Dependencies (`dependencies/`)

Bundled as submodules and built in-tree: `libosmscout` (core), `libsailfishapp`,
`marisa-trie` plus TTS stack
`espeak-ng` / `piper1-gpl` / `onnxruntime` (guarded by `PIPER_FOUND`). Prefer changing
`libosmscout` upstream over patching around it here.

