# ==================================================================================================
# libmarisa

# libmarisa don't have cmake based build, but it is relatively small,
# so we define library target here

set(MARISA_ROOT ${CMAKE_HOME_DIRECTORY}/dependencies/marisa-trie)
set(MARISA_INCLUDE_DIRS
        ${MARISA_ROOT}/lib
        ${MARISA_ROOT}/include)

set(MARISA_SRC
        ${MARISA_ROOT}/lib/marisa/agent.cc
        ${MARISA_ROOT}/lib/marisa/exception.cc
        ${MARISA_ROOT}/lib/marisa/grimoire/io/mapper.cc
        ${MARISA_ROOT}/lib/marisa/grimoire/io/reader.cc
        ${MARISA_ROOT}/lib/marisa/grimoire/io/writer.cc
        ${MARISA_ROOT}/lib/marisa/grimoire/trie/louds-trie.cc
        ${MARISA_ROOT}/lib/marisa/grimoire/trie/tail.cc
        ${MARISA_ROOT}/lib/marisa/grimoire/vector/bit-vector.cc
        ${MARISA_ROOT}/lib/marisa/trie.cc
        ${MARISA_ROOT}/lib/marisa/keyset.cc
)

include_directories(
        ${MARISA_INCLUDE_DIRS}
)

add_library(marisa ${MARISA_SRC})
set_property(TARGET marisa PROPERTY CXX_STANDARD 20)

install(TARGETS marisa
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# skip marisa autodetection and setup these properties explicitly

# This one is used by `create_private_config` function (features.cmake)
# for generating private Config.h from template Config.h.cmake
set(OSMSCOUT_HAVE_LIB_MARISA TRUE PARENT_SCOPE)

# This one is used by other CMakeLists.txt files for add Marisa-specific classes
set(MARISA_FOUND TRUE PARENT_SCOPE)

# used by linker
set(MARISA_LIBRARIES marisa PARENT_SCOPE)

# ==================================================================================================
# OSM Scout library

set(OSMSCOUT_BASE_DIR_BUILD ${CMAKE_CURRENT_BINARY_DIR})
set(OSMSCOUT_BASE_DIR_SOURCE ${CMAKE_HOME_DIRECTORY}/dependencies/libosmscout)
set(OSMSCOUT_BASE_DIR_BUILD ${CMAKE_CURRENT_BINARY_DIR}/dependencies/libosmscout)

set(OSMSCOUT_INCLUDE_DIRS
        ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
        ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
        ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include

        ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-gpx/include
        ${OSMSCOUT_BASE_DIR_BUILD}/libosmscout-gpx/include

        ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-client-qt/include
)

# we are including libosmscout subdirectories directly, we are responsible
# to setup all variables correctly
include(libosmscout/cmake/ProjectConfig.cmake)

add_subdirectory(libosmscout/libosmscout)
add_subdirectory(libosmscout/libosmscout-map)
add_subdirectory(libosmscout/libosmscout-map-qt)
add_subdirectory(libosmscout/libosmscout-gpx)
add_subdirectory(libosmscout/libosmscout-client)
add_subdirectory(libosmscout/libosmscout-client-qt)

set_property(TARGET OSMScout         PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
set_property(TARGET OSMScoutMap      PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
set_property(TARGET OSMScoutMapQt    PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
set_property(TARGET OSMScoutGPX      PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
set_property(TARGET OSMScoutClientQt PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})

target_compile_definitions(OSMScoutClientQt PRIVATE LIBOSMSCOUT_VERSION_STRING="${LIBOSMSCOUT_GIT_HASH}")

set(OSMSCOUT_BASE_DIR_SOURCE ${OSMSCOUT_BASE_DIR_SOURCE} PARENT_SCOPE)
set(OSMSCOUT_INCLUDE_DIRS ${OSMSCOUT_INCLUDE_DIRS} PARENT_SCOPE)

# ==================================================================================================
# Piper TTS stack (espeak-ng + onnxruntime + libpiper)
#
# These are third-party projects (not "buddy" projects like libosmscout / marisa),
# so we build them as isolated ExternalProjects from their vendored submodules.
#
#  - espeak-ng   : built static, linked into libpiper
#  - onnxruntime : built as a shared library and bundled into the app private lib dir
#                  (Harbour allows bundling otherwise-disallowed .so files there).
#                  Building it fully static would require resolving ~13 transitive
#                  archives + a find_package() superbuild, which libpiper does not expect.
#  - libpiper    : single translation unit (piper.cpp) exposing a C API, built static here

include(ExternalProject)

option(BUILD_PIPER_TTS "Build the vendored Piper TTS stack (espeak-ng, onnxruntime, libpiper)" ON)

if (BUILD_PIPER_TTS)

  set(PIPER_ROOT       ${CMAKE_HOME_DIRECTORY}/dependencies/piper1-gpl/libpiper)
  set(ESPEAKNG_SOURCE  ${CMAKE_HOME_DIRECTORY}/dependencies/espeak-ng)
  set(ONNXRUNTIME_SOURCE ${CMAKE_HOME_DIRECTORY}/dependencies/onnxruntime)

  # ------------------------------------------------------------------------------------------------
  # espeak-ng (static)

  set(ESPEAKNG_PREFIX       ${CMAKE_CURRENT_BINARY_DIR}/espeak-ng)
  set(ESPEAKNG_BINARY_DIR   ${ESPEAKNG_PREFIX}/build)
  set(ESPEAKNG_INSTALL_DIR  ${ESPEAKNG_PREFIX}/install)
  set(ESPEAKNG_STATIC_LIB   ${ESPEAKNG_INSTALL_DIR}/lib/libespeak-ng.a)
  set(UCD_STATIC_LIB        ${ESPEAKNG_BINARY_DIR}/src/ucd-tools/libucd.a)
  set(ESPEAKNG_DATA_DIR     ${ESPEAKNG_INSTALL_DIR}/share/espeak-ng-data)

  ExternalProject_Add(espeak_ng_external
      SOURCE_DIR       ${ESPEAKNG_SOURCE}
      DOWNLOAD_COMMAND ""
      PREFIX           ${ESPEAKNG_PREFIX}
      BINARY_DIR       ${ESPEAKNG_BINARY_DIR}
      INSTALL_DIR      ${ESPEAKNG_INSTALL_DIR}
      CMAKE_ARGS
          -DCMAKE_INSTALL_PREFIX=${ESPEAKNG_INSTALL_DIR}
          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
          -DBUILD_SHARED_LIBS:BOOL=OFF
          -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
          -DUSE_ASYNC:BOOL=OFF
          -DUSE_MBROLA:BOOL=OFF
          -DUSE_LIBSONIC:BOOL=OFF
          -DUSE_LIBPCAUDIO:BOOL=OFF
          -DUSE_KLATT:BOOL=OFF
          -DUSE_SPEECHPLAYER:BOOL=OFF
          -DEXTRA_cmn:BOOL=ON
          -DEXTRA_ru:BOOL=ON
          "-DCMAKE_C_FLAGS=-D_FILE_OFFSET_BITS=64 -I${ESPEAKNG_SOURCE}/src/ucd-tools/src/include"
          "-DCMAKE_CXX_FLAGS=-D_FILE_OFFSET_BITS=64 -I${ESPEAKNG_SOURCE}/src/ucd-tools/src/include"
      BUILD_BYPRODUCTS
          ${ESPEAKNG_STATIC_LIB}
          ${UCD_STATIC_LIB}
  )

  # ------------------------------------------------------------------------------------------------
  # onnxruntime (shared, bundled)

  set(ONNXRUNTIME_PREFIX      ${CMAKE_CURRENT_BINARY_DIR}/onnxruntime)
  set(ONNXRUNTIME_BINARY_DIR  ${ONNXRUNTIME_PREFIX}/build)
  set(ONNXRUNTIME_INSTALL_DIR ${ONNXRUNTIME_PREFIX}/install)
  set(ONNXRUNTIME_INCLUDE_DIR ${ONNXRUNTIME_INSTALL_DIR}/include)
  set(ONNXRUNTIME_SHARED_LIB  ${ONNXRUNTIME_INSTALL_DIR}/lib/libonnxruntime.so)

  # onnxruntime translation units (onnx, protobuf, abseil and especially the CPU
  # provider kernels such as element_wise_ops.cc) are extremely memory hungry:
  # at -O3 a single file can need ~1.5-2.5 GB of RAM. Building more than one
  # such file at a time exhausts memory ("virtual memory exhausted: Cannot allocate memory").
  # Build the onnxruntime sub-tree strictly serially.
  set(ONNXRUNTIME_BUILD_PARALLEL_LEVEL "1" CACHE STRING
          "Maximum number of parallel compile jobs for the onnxruntime sub-build")

  # Even a single onnxruntime file at -O3 can spike to >2.5 GB. Make GCC's
  # garbage collector far more aggressive so it frees memory during compilation
  # instead of growing the heap unbounded. This cuts peak memory dramatically
  # (at a small build-time cost) and keeps even the heaviest kernels within the
  # 3.5 GB / no-swap budget. These --param flags are honoured regardless of the
  # -O level onnxruntime selects.
  set(ONNXRUNTIME_LOW_MEM_CXX_FLAGS
          "--param ggc-min-expand=10 --param ggc-min-heapsize=32768")

  ExternalProject_Add(onnxruntime_external
      SOURCE_DIR       ${ONNXRUNTIME_SOURCE}/cmake
      DOWNLOAD_COMMAND ""
      PREFIX           ${ONNXRUNTIME_PREFIX}
      BINARY_DIR       ${ONNXRUNTIME_BINARY_DIR}
      INSTALL_DIR      ${ONNXRUNTIME_INSTALL_DIR}
      CMAKE_ARGS
          -DCMAKE_INSTALL_PREFIX=${ONNXRUNTIME_INSTALL_DIR}
          # Always build onnxruntime in Release (no -g): it is a vendored
          # dependency we do not debug, and the debug info on huge generated
          # files such as onnx/version_converter/convert.cc triggers an
          # assembler ".gcc_except_table" error / cc1plus crash on the aarch64
          # toolchain. Release also reduces build time and memory usage.
          -DCMAKE_BUILD_TYPE=Release
          -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
          -DCMAKE_C_FLAGS=${ONNXRUNTIME_LOW_MEM_CXX_FLAGS}
          -DCMAKE_CXX_FLAGS=${ONNXRUNTIME_LOW_MEM_CXX_FLAGS}
          -Donnxruntime_BUILD_SHARED_LIB:BOOL=ON
          -Donnxruntime_BUILD_UNIT_TESTS:BOOL=OFF
          -Donnxruntime_USE_FULL_PROTOBUF:BOOL=ON
      BUILD_COMMAND
          ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
              --parallel ${ONNXRUNTIME_BUILD_PARALLEL_LEVEL}
      BUILD_BYPRODUCTS
          ${ONNXRUNTIME_SHARED_LIB}
  )

  # onnxruntime installs the C/C++ API headers in a few nested directories
  set(ONNXRUNTIME_INCLUDE_DIRS
      ${ONNXRUNTIME_INCLUDE_DIR}
      ${ONNXRUNTIME_INCLUDE_DIR}/onnxruntime
      ${ONNXRUNTIME_INCLUDE_DIR}/onnxruntime/core/session
  )

  # ------------------------------------------------------------------------------------------------
  # libpiper (static) - single translation unit exposing the C API

  add_library(piper STATIC ${PIPER_ROOT}/src/piper.cpp)
  set_property(TARGET piper PROPERTY CXX_STANDARD 17)
  add_dependencies(piper espeak_ng_external onnxruntime_external)

  target_include_directories(piper PUBLIC
      ${PIPER_ROOT}/include
      ${ESPEAKNG_INSTALL_DIR}/include
      ${ONNXRUNTIME_INCLUDE_DIRS}
  )

  target_link_libraries(piper PUBLIC
      ${ESPEAKNG_STATIC_LIB}
      ${UCD_STATIC_LIB}
      ${ONNXRUNTIME_SHARED_LIB}
  )

  # exported for the application / test binaries and packaging
  set(PIPER_FOUND TRUE PARENT_SCOPE)
  set(PIPER_LIBRARIES piper PARENT_SCOPE)
  set(PIPER_INCLUDE_DIRS ${PIPER_ROOT}/include PARENT_SCOPE)
  set(PIPER_ONNXRUNTIME_SHARED_LIB ${ONNXRUNTIME_SHARED_LIB} PARENT_SCOPE)
  set(PIPER_ESPEAKNG_DATA_DIR ${ESPEAKNG_DATA_DIR} PARENT_SCOPE)

endif()

