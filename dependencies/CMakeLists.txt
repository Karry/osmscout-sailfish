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
