#
# Copyright (c) 2016 Lukas Karas <lukas.karas@centrum.cz>
#
# Structure of OSMScout-Sailfish cmake scripts was inspired by DigiKam project
#
# Redistribution and use is allowed according to the terms of the BSD license.

project(osmscout-sailfish C CXX)

# ==================================================================================================
# Information to update before to release this package.

# project version
set(OSMSCOUT_SAILFISH_MAJOR_VERSION "2")
set(OSMSCOUT_SAILFISH_MINOR_VERSION "3")
set(OSMSCOUT_SAILFISH_PATCH_VERSION "0")

set(OSMSCOUT_SAILFISH_SUFFIX_VERSION "")
set(OSMSCOUT_SAILFISH_VERSION_SHORT  "${OSMSCOUT_SAILFISH_MAJOR_VERSION}.${OSMSCOUT_SAILFISH_MINOR_VERSION}.${OSMSCOUT_SAILFISH_PATCH_VERSION}")
set(OSMSCOUT_SAILFISH_VERSION_STRING "${OSMSCOUT_SAILFISH_VERSION_SHORT}${OSMSCOUT_SAILFISH_SUFFIX_VERSION}")

execute_process(COMMAND git rev-parse --short HEAD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies/libosmscout
                OUTPUT_VARIABLE LIBOSMSCOUT_GIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE GIT_RETURN_CODE)

if(NOT GIT_RETURN_CODE EQUAL "0")
    message(FATAL_ERROR "git command failed")
endif()

message(STATUS "libosmscout Git hash: ${LIBOSMSCOUT_GIT_HASH}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CMakeMod/Version.h.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/privateinclude/harbour-osmscout/private/Version.h")

# ==================================================================================================

set(CMAKE_MIN_VERSION    "3.9.2") # CMake availabe on Travis CI is 3.9.2
set(QT_MIN_VERSION       "5.6.0") # QList::constBegin was introduced in Qt 5.6
option(OSMSCOUT_ENABLE_SSE "Enable SSE support (not working on all platforms!)" ON)

cmake_minimum_required(VERSION ${CMAKE_MIN_VERSION})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_HOME_DIRECTORY}/CMakeMod")

# Cmake macros
include(FeatureSummary)

# Local macros
include(MacroUtils)

# feature detection (libraries, header, etc.)
set(OSMSCOUT_ENABLE_SSE true)
include(CheckCXXCompilerFlag)
include(features)

# ==================================================================================================
# Find dependencies

find_package( Qt5 ${QT_MIN_VERSION}
              REQUIRED NO_MODULE COMPONENTS
              Core
              DBus
              Gui
              Multimedia
              Network
              Positioning
              Qml
              Quick
              Sql
              Svg
              Xml
)
find_package(Qt5LinguistTools)
find_package(OpenMP REQUIRED)
find_package(SailfishApp) # https://github.com/sailfish-sdk/libsailfishapp

# ==================================================================================================

option(DEBUG_LABEL_LAYOUTER "Print extra debug messages during label layouting" OFF)
if (LABEL_LAYOUTER_DEBUG)
    add_definitions( -DLABEL_LAYOUTER_DEBUG)
else()
    set(LABEL_LAYOUTER_DEBUG FALSE)
endif()

option(DEBUG_GROUNDTILES "Print extra debug messages while rendering ground tiles" OFF)
if (DEBUG_GROUNDTILES)
    add_definitions( -DDEBUG_GROUNDTILES)
else()
    set(DEBUG_GROUNDTILES FALSE)
endif()

# see https://doc.qt.io/qtcreator/creator-debugging-qml.html for more details
option(QT_QML_DEBUG "Build with QML debugger support" OFF)
if (QT_QML_DEBUG)
    add_definitions( -DQT_QML_DEBUG)
else()
    set(QT_QML_DEBUG FALSE)
endif()

option(SANITIZER "Build with sanitizer" none)
if(SANITIZER STREQUAL "address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
elseif(SANITIZER STREQUAL "undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
elseif(SANITIZER STREQUAL "thread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
else()
    set(SANITIZER "none")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)
if (NOT BUILD_TYPE_LOWER STREQUAL "debug")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result OUTPUT output)
    if(result)
        set(OSMSCOUT_ENABLE_IPO TRUE)
    else()
        message(WARNING "IPO is not supported")
        set(OSMSCOUT_ENABLE_IPO FALSE)
    endif()
else()
    set(OSMSCOUT_ENABLE_IPO FALSE)
endif()

add_definitions( -Wall -Wextra -pedantic -Wno-unused-function )
add_definitions( -fno-omit-frame-pointer -fstack-protector-all -fPIC )
set(CMAKE_CXX_STANDARD_REQUIRED ON) # build should fail when compiler don't support standard defined by CMAKE_CXX_STANDARD

# Do *NOT* turn assertions off in release and rel-with-deb-info modes.
# CMakes does that by default despite any recent documentation
# recommending against the practice.
string( REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
string( REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
string( REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
string( REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

# add function name to Qt log messages (even in release build)
add_definitions( -DQT_MESSAGELOGCONTEXT )

add_definitions(${LIBXML2_DEFINITIONS})
add_definitions(${LIBSAILFISHAPP_DEFINITIONS})

# OpenMP definitions
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
set(OSMSCOUT_HAVE_OPENMP ${OPENMP_FOUND})

# CMake doesn't add "-pie" by default for executables (CMake issue #14983)
# it is necessary for accelerated start by mapplauncherd,
# see https://git.sailfishos.org/mer-core/mapplauncherd for details
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")

set(HARBOUR_APP_NAME harbour-osmscout)

# ==================================================================================================
# Print build configuration

## print all variables
#message(STATUS "contractionhierarchies: "contractionhierarchies)
#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

message(STATUS "")
message(STATUS "----------------------------------------------------------------------------------")
message(STATUS "OSMScout-Sailfish ${OSMSCOUT_SAILFISH_VERSION_STRING} build configuration results")

message(STATUS "")
message(STATUS "Build options:")

message(STATUS " Build type .............................. ${CMAKE_BUILD_TYPE}")
message(STATUS " Interprocedural optimizations ........... ${OSMSCOUT_ENABLE_IPO}")
message(STATUS " Compiler flags .......................... ${CMAKE_CXX_FLAGS}")
message(STATUS " Linker flags ............................ ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS " Sanitizer ............................... ${SANITIZER}")
message(STATUS " LABEL_LAYOUTER_DEBUG .................... ${LABEL_LAYOUTER_DEBUG}")
message(STATUS " DEBUG_GROUNDTILES ....................... ${DEBUG_GROUNDTILES}")
message(STATUS " QT_QML_DEBUG ............................ ${QT_QML_DEBUG}")

message(STATUS "")
message(STATUS "Requiered dependencies:")

PRINT_LIBRARY_STATUS("Qt5"                    "http://www.qtsoftware.com"                       "(version >= ${QT_MIN_VERSION})"     Qt5_FOUND)
#PRINT_LIBRARY_STATUS("libprotobuf"            "https://github.com/google/protobuf"             ""      PROTOBUF_FOUND)
PRINT_LIBRARY_STATUS("OpenMP compiler support" "http://openmp.org/wp/openmp-compilers/"         ""      OPENMP_FOUND)
PRINT_LIBRARY_STATUS("SailfishApp library"     "https://github.com/sailfish-sdk/libsailfishapp" ""      LIBSAILFISHAPP_FOUND)
PRINT_LIBRARY_STATUS("Xml2 library"            "http://xmlsoft.org/"                            ""      LIBXML2_FOUND)

message(STATUS "")

if(Qt5_FOUND AND
        LIBSAILFISHAPP_FOUND AND
        LIBXML2_FOUND
        )
    message(STATUS      " OSMScout-Sailfish can be compiled ....... YES")
else()
    message(FATAL_ERROR " OSMScout-Sailfish will be compiled ...... NO  (Look README file for more details about dependencies)")
endif()

message(STATUS "----------------------------------------------------------------------------------")
message(STATUS "")

# ==================================================================================================
# libmarisa

# libmarisa don't have cmake based build, but it is relatively small,
# so we define library target here

set(MARISA_ROOT ${CMAKE_HOME_DIRECTORY}/dependencies/marisa-trie)
set(MARISA_INCLUDE_DIRS
    ${MARISA_ROOT}/lib
    ${MARISA_ROOT}/include)

set(MARISA_SRC
    ${MARISA_ROOT}/lib/marisa/grimoire/io/reader.cc
    ${MARISA_ROOT}/lib/marisa/grimoire/io/mapper.cc
    ${MARISA_ROOT}/lib/marisa/grimoire/io/writer.cc
    ${MARISA_ROOT}/lib/marisa/grimoire/trie/tail.cc
    ${MARISA_ROOT}/lib/marisa/grimoire/trie/louds-trie.cc
    ${MARISA_ROOT}/lib/marisa/grimoire/vector/bit-vector.cc
    ${MARISA_ROOT}/lib/marisa/trie.cc
    ${MARISA_ROOT}/lib/marisa/agent.cc
    ${MARISA_ROOT}/lib/marisa/keyset.cc
)

include_directories(
  ${MARISA_INCLUDE_DIRS}
)

add_library(marisa
    ${MARISA_SRC}
)
install(TARGETS marisa
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# skip marisa autodetection and setup these properties explicitly

# This one is used by `create_private_config` function (features.cmake)
# for generating private Config.h from template Config.h.cmake
set(OSMSCOUT_HAVE_LIB_MARISA TRUE)

# This one is used by other CMakeLists.txt files for add Marisa-specific classes
set(MARISA_FOUND TRUE)

# used by linker
set(MARISA_LIBRARIES marisa)

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
add_subdirectory(dependencies/libosmscout/libosmscout)
add_subdirectory(dependencies/libosmscout/libosmscout-map)
add_subdirectory(dependencies/libosmscout/libosmscout-map-qt)
add_subdirectory(dependencies/libosmscout/libosmscout-gpx)
add_subdirectory(dependencies/libosmscout/libosmscout-client-qt)

set_property(TARGET OSMScout         PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
set_property(TARGET OSMScoutMap      PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
set_property(TARGET OSMScoutMapQt    PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
set_property(TARGET OSMScoutGPX      PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
set_property(TARGET OSMScoutClientQt PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})

# ==================================================================================================
# harbour-osmscout

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
    src/AppSettings.h
    src/Arguments.h
    src/Storage.h
    src/CollectionModel.h
    src/CollectionListModel.h
    src/QVariantConverters.h
    src/CollectionTrackModel.h
    src/CollectionMapBridge.h
    src/IconProvider.h
    src/LocFile.h
    src/MemoryManager.h
    src/Tracker.h
    src/SearchHistoryModel.h)

# keep qml files in source list - it makes qtcreator happy
# find qml -type f
set(QML_FILES
    qml/AboutDialog.qml
    qml/SearchDialog.qml

    qml/custom/DialogActionButton.qml
    qml/custom/LineEdit.qml
    qml/custom/LocationSearch.qml
    qml/custom/MapButton.qml
    qml/custom/MapDialog.qml
    qml/custom/ScrollIndicator.qml
    qml/custom/MapRenderingIndicator.qml
    qml/custom/Link.qml
    qml/custom/OSMCopyright.qml
    qml/custom/POIIcon.qml
    qml/custom/AvailableMapsView.qml
    qml/custom/MapBase.qml
    qml/custom/MapComponent.qml
    qml/custom/ScaleIndicator.qml
    qml/custom/LocationSelector.qml
    qml/custom/RoutingStep.qml
    qml/custom/RouteStepIcon.qml
    qml/custom/LaneTurnIcon.qml
    qml/custom/CollectionEditDialog.qml
    qml/custom/CollectionSelector.qml
    qml/custom/Global.qml
    qml/custom/Utils.js
    qml/custom/SpeedIndicator.qml
    qml/custom/WebsiteRow.qml
    qml/custom/PhoneRow.qml
    qml/custom/CollectionComboBox.qml
    qml/custom/CollectionEntryDialog.qml
    qml/custom/LaneTurns.qml

    qml/pages/Cover.qml
    qml/pages/Search.qml
    qml/pages/MapList.qml
    qml/pages/Downloads.qml
    qml/pages/MapDetail.qml
    qml/pages/About.qml
    qml/pages/PlaceDetail.qml
    qml/pages/MapPage.qml
    qml/pages/NavigationInstructions.qml
    qml/pages/Layers.qml
    qml/pages/MapObjects.qml
    qml/pages/Routing.qml
    qml/pages/RouteDescription.qml
    qml/pages/Collections.qml
    qml/pages/Collection.qml
    qml/pages/CollectionTrack.qml
    qml/pages/CollectionWaypoint.qml
    qml/pages/NewWaypoint.qml
    qml/pages/CollectionExport.qml
    qml/pages/Settings.qml
    qml/pages/Tracker.qml
    qml/pages/VoiceSelector.qml
    qml/main.qml
    qml/l10n.qml
    qml/desktop.qml)

set(SOURCE_FILES
    src/AppSettings.cpp
    src/LocFile.cpp
    src/MemoryManager.cpp
    src/OSMScout.cpp
    src/Storage.cpp
    src/CollectionModel.cpp
    src/CollectionListModel.cpp
    src/CollectionTrackModel.cpp
    src/CollectionMapBridge.cpp
    src/Tracker.cpp
    src/SearchHistoryModel.cpp)

# XML files with translated phrases.
# You can add new language translation just by adding new entry here, and run build.
# Newly created *.ts file may be edited by linguist tool.
set(TRANSLATION_TS_FILES
        translations/en.ts

        translations/cs.ts
        translations/de.ts
        translations/es.ts
        translations/fa.ts
        translations/fr.ts
        translations/hu.ts
        translations/it.ts
        translations/nb.ts
        translations/nl.ts
        translations/nl_BE.ts
        translations/pl.ts
        translations/pt_BR.ts
        translations/ru.ts
        translations/sv.ts
        translations/zh_CN.ts
        )

# source files that may contains translatable phrases (see tr, qsTr, QT_TRANSLATE_NOOP functions)
set(TRANSLATION_SOURCE_FILES
        ${SOURCE_FILES}
        ${QML_FILES}
        dependencies/libosmscout/libosmscout-client-qt/src/osmscout/RouteDescriptionBuilder.cpp
        )

# See http://doc.qt.io/qt-5/cmake-manual.html
#
# qt5_create_translation macro process translations in two steps:
#
#  - collect available phrases inside TRANSLATION_SOURCE_FILES
#    with lupdate tool and put them into *.ts files if some is missing there.
#
#  - create binary translation files (*.qm) from translated phrases available
#    in *.ts files with lrelease tool
qt5_create_translation(TRANSLATION_QM_FILES
        ${TRANSLATION_SOURCE_FILES}
        ${TRANSLATION_TS_FILES})

add_custom_target(translations ALL DEPENDS ${TRANSLATION_QM_FILES})

add_executable(harbour-osmscout ${SOURCE_FILES} ${HEADER_FILES} ${QML_FILES})
set_property(TARGET harbour-osmscout PROPERTY CXX_STANDARD 17)
set_property(TARGET harbour-osmscout PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
add_dependencies(harbour-osmscout translations)

# Private config header
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CMakeMod/Config.h.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/privateinclude/harbour-osmscout/private/Config.h")

target_include_directories(harbour-osmscout PRIVATE 
        ${CMAKE_CURRENT_BINARY_DIR}/privateinclude
        ${OSMSCOUT_INCLUDE_DIRS}
        ${LIBSAILFISHAPP_INCLUDE_DIRS}
        ${LIBXML2_INCLUDE_DIR}
)

target_link_libraries(harbour-osmscout
        Qt5::Core
        Qt5::DBus
        Qt5::Gui
        Qt5::Qml
        Qt5::Quick
        Qt5::Svg
        Qt5::Positioning
        Qt5::Sql

        marisa
        OSMScout
        OSMScoutMap
        OSMScoutMapQt
        OSMScoutGPX
        OSMScoutClientQt
        ${LIBSAILFISHAPP_LIBRARIES}
        ${LIBXML2_LIBRARIES}
)

# https://github.com/sailfish-sdk/cmakesample/blob/master/CMakeLists.txt
install(TARGETS harbour-osmscout
RUNTIME DESTINATION bin)

install(DIRECTORY qml
DESTINATION share/${HARBOUR_APP_NAME})

install(DIRECTORY resources
DESTINATION share/${HARBOUR_APP_NAME})

install(DIRECTORY pics
DESTINATION share/${HARBOUR_APP_NAME})

install(FILES ${TRANSLATION_QM_FILES}
DESTINATION share/${HARBOUR_APP_NAME}/translations)

install(FILES harbour-osmscout.desktop
DESTINATION share/applications)

install(FILES icons/86x86/harbour-osmscout.png
DESTINATION share/icons/hicolor/86x86/apps)

install(FILES icons/108x108/harbour-osmscout.png
DESTINATION share/icons/hicolor/108x108/apps)

install(FILES icons/128x128/harbour-osmscout.png
DESTINATION share/icons/hicolor/128x128/apps)

install(FILES icons/256x256/harbour-osmscout.png
DESTINATION share/icons/hicolor/256x256/apps)

install(FILES icons/172x172/harbour-osmscout.png
DESTINATION share/icons/hicolor/172x172/apps)

install(DIRECTORY icons/poi/
        DESTINATION share/${HARBOUR_APP_NAME}/poi-icons
        FILES_MATCHING PATTERN "*.svg")

install(DIRECTORY icons/routestep/
        DESTINATION share/${HARBOUR_APP_NAME}/routestep
        FILES_MATCHING PATTERN "*.svg")

install(DIRECTORY dependencies/libosmscout/libosmscout/data/icons/svg/standard/
        DESTINATION share/${HARBOUR_APP_NAME}/map-icons
        FILES_MATCHING PATTERN "*.svg")

install(FILES
        stylesheets/boundaries.oss
        stylesheets/public-transport.oss
        stylesheets/standard.oss
        stylesheets/outdoor.oss
        stylesheets/winter-sports.oss
DESTINATION share/${HARBOUR_APP_NAME}/map-styles)

# ==================================================================================================
# PerformanceTest binary
set(SOURCE_FILES
    src/PerformanceTest.cpp
)

add_executable(PerformanceTest ${SOURCE_FILES})
set_property(TARGET PerformanceTest PROPERTY CXX_STANDARD 17)

target_include_directories(PerformanceTest PRIVATE 
        ${OSMSCOUT_INCLUDE_DIRS}
        ${LIBSAILFISHAPP_INCLUDE_DIRS}
)

target_link_libraries(PerformanceTest
        Qt5::Core
        Qt5::Gui
        Qt5::Quick # requiered by sailfishapp when configred with QT_QML_DEBUG

        marisa
        OSMScout
        OSMScoutMap
        OSMScoutMapQt
        ${LIBSAILFISHAPP_LIBRARIES}
)

# ==================================================================================================
# SearchPerfTest binary
set(SOURCE_FILES
        src/SearchPerfTest.cpp
        )

add_executable(SearchPerfTest ${SOURCE_FILES})
set_property(TARGET SearchPerfTest PROPERTY CXX_STANDARD 17)

target_include_directories(SearchPerfTest PRIVATE
        ${OSMSCOUT_INCLUDE_DIRS}
        ${LIBSAILFISHAPP_INCLUDE_DIRS}
        )

target_link_libraries(SearchPerfTest
        Qt5::Core
        Qt5::Gui
        Qt5::Quick # requiered by sailfishapp when configred with QT_QML_DEBUG

        marisa
        OSMScout
        OSMScoutMap
        OSMScoutMapQt
        ${LIBSAILFISHAPP_LIBRARIES}
        )

# ==================================================================================================
# MultiDBRouting

add_executable(MultiDBRouting ${OSMSCOUT_BASE_DIR_SOURCE}/Tests/src/MultiDBRouting.cpp)
set_property(TARGET MultiDBRouting PROPERTY CXX_STANDARD 17)
target_include_directories(MultiDBRouting PRIVATE ${OSMSCOUT_INCLUDE_DIRS})
target_link_libraries(MultiDBRouting
        OSMScout
        )

# ==================================================================================================
# Routing

add_executable(Routing ${OSMSCOUT_BASE_DIR_SOURCE}/Demos/src/Routing.cpp)
set_property(TARGET Routing PROPERTY CXX_STANDARD 17)
target_include_directories(Routing PRIVATE ${OSMSCOUT_INCLUDE_DIRS})
target_link_libraries(Routing
        OSMScout
        )
