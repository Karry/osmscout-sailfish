#
# Copyright (c) 2016 Lukas Karas <lukas.karas@centrum.cz>
#
# Structure of OSMScout-Sailfish cmake scripts was inspired by DigiKam project
#
# Redistribution and use is allowed according to the terms of the BSD license.
cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0048 NEW)
project(osmscout-sailfish VERSION 2.39.0 LANGUAGES C CXX)

# ==================================================================================================
# Information to update before to release this package.

# project version
set(OSMSCOUT_SAILFISH_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set(OSMSCOUT_SAILFISH_MINOR_VERSION ${PROJECT_VERSION_MINOR})
set(OSMSCOUT_SAILFISH_PATCH_VERSION ${PROJECT_VERSION_PATCH})

set(OSMSCOUT_SAILFISH_SUFFIX_VERSION "")
set(OSMSCOUT_SAILFISH_VERSION_SHORT  "${OSMSCOUT_SAILFISH_MAJOR_VERSION}.${OSMSCOUT_SAILFISH_MINOR_VERSION}.${OSMSCOUT_SAILFISH_PATCH_VERSION}")
set(OSMSCOUT_SAILFISH_VERSION_STRING "${OSMSCOUT_SAILFISH_VERSION_SHORT}${OSMSCOUT_SAILFISH_SUFFIX_VERSION}")

execute_process(COMMAND git rev-parse --short HEAD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies/libosmscout
                OUTPUT_VARIABLE LIBOSMSCOUT_GIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE GIT_RETURN_CODE)

if(NOT GIT_RETURN_CODE EQUAL "0")
    message(WARNING "git command failed")
    set(LIBOSMSCOUT_GIT_HASH "?")
endif()
set(OSMSCOUT_LIBRARY_VERSION ${LIBOSMSCOUT_GIT_HASH})

message(STATUS "libosmscout Git hash: ${LIBOSMSCOUT_GIT_HASH}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CMakeMod/Version.h.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/privateinclude/harbour-osmscout/private/Version.h")

# ==================================================================================================

set(CMAKE_MIN_VERSION    "3.19.3") # Sailfish SDK has CMake 3.19.3
set(QT_MIN_VERSION       "5.6.0") # Sailfish OS is build on Qt/5.6.3
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
              LinguistTools
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

# global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # build should fail when compiler don't support standard defined by CMAKE_CXX_STANDARD
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE OFF)

add_definitions( -Wall -Wextra -pedantic -Wno-unused-function )
add_definitions( -fno-omit-frame-pointer -fstack-protector-all -fPIC )

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
include(dependencies/libosmscout/cmake/ProjectConfig.cmake)

add_subdirectory(dependencies/libosmscout/libosmscout)
add_subdirectory(dependencies/libosmscout/libosmscout-map)
add_subdirectory(dependencies/libosmscout/libosmscout-map-qt)
add_subdirectory(dependencies/libosmscout/libosmscout-gpx)
add_subdirectory(dependencies/libosmscout/libosmscout-client)
add_subdirectory(dependencies/libosmscout/libosmscout-client-qt)

set_property(TARGET OSMScout         PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
set_property(TARGET OSMScoutMap      PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
set_property(TARGET OSMScoutMapQt    PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
set_property(TARGET OSMScoutGPX      PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})
set_property(TARGET OSMScoutClientQt PROPERTY INTERPROCEDURAL_OPTIMIZATION ${OSMSCOUT_ENABLE_IPO})

target_compile_definitions(OSMScoutClientQt PRIVATE LIBOSMSCOUT_VERSION_STRING="${LIBOSMSCOUT_GIT_HASH}")

# ==================================================================================================
# harbour-osmscout

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
    src/AppSettings.h
    src/Arguments.h
    src/Storage.h
    src/CollectionModel.h
    src/CollectionStatisticsModel.h
    src/CollectionListModel.h
    src/QVariantConverters.h
    src/CollectionTrackModel.h
    src/CollectionMapBridge.h
    src/IconProvider.h
    src/LocFile.h
    src/MemoryManager.h
    src/NearWaypointModel.h
    src/Tracker.h
    src/SearchHistoryModel.h
    src/PositionSimulator.h
    )

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
    qml/custom/OpeningHoursRow.qml
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
    qml/custom/GpsFixWarning.qml
    qml/custom/Utils.js
    qml/custom/SpeedIndicator.qml
    qml/custom/WebsiteRow.qml
    qml/custom/PhoneRow.qml
    qml/custom/CollectionComboBox.qml
    qml/custom/CollectionEntryDialog.qml
    qml/custom/LaneTurns.qml
    qml/custom/RouteElevationChart.qml
    qml/custom/RouteProfileMenuItem.qml
    qml/custom/SymbolSelector.qml
    qml/custom/TrackTypeComboBox.qml
    qml/custom/SymbolIcon.qml
    qml/custom/TrackTypeMenuItem.qml
    qml/custom/TrackTypes.qml
    qml/custom/TrackTypeIcon.qml

    qml/pages/Cover.qml
    qml/pages/Search.qml
    qml/pages/MapList.qml
    qml/pages/DatabaseWarning.qml
    qml/pages/Downloads.qml
    qml/pages/MapDetail.qml
    qml/pages/About.qml
    qml/pages/PlaceDetail.qml
    qml/pages/PlacePicker.qml
    qml/pages/MapPage.qml
    qml/pages/NavigationInstructions.qml
    qml/pages/Layers.qml
    qml/pages/MapObjects.qml
    qml/pages/Routing.qml
    qml/pages/RouteDescription.qml
    qml/pages/Collections.qml
    qml/pages/Collection.qml
    qml/pages/CollectionStatistics.qml
    qml/pages/CollectionTrack.qml
    qml/pages/CollectionWaypoint.qml
    qml/pages/CollectionOrdering.qml
    qml/pages/CollectionListOrdering.qml
    qml/pages/NewWaypoint.qml
    qml/pages/OpeningHours.qml
    qml/pages/CollectionExport.qml
    qml/pages/Settings.qml
    qml/pages/TrackColor.qml
    qml/pages/Tracker.qml
    qml/pages/TrackEdit.qml
    qml/pages/TrackFilter.qml
    qml/pages/VoiceSelector.qml
    qml/pages/WaypointSelector.qml
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
    src/CollectionStatisticsModel.cpp
    src/CollectionListModel.cpp
    src/CollectionTrackModel.cpp
    src/CollectionMapBridge.cpp
    src/TrackElevationChartWidget.cpp
    src/Tracker.cpp
    src/SearchHistoryModel.cpp
    src/NearWaypointModel.cpp
    src/PositionSimulator.cpp)

# XML files with translated phrases.
# You can add new language translation just by adding new entry here, and run build.
# Newly created *.ts file may be edited by linguist tool.
set(TRANSLATION_TS_FILES
        translations/en.ts

        translations/cs.ts
        translations/de.ts
        translations/es.ts
        translations/et.ts
        translations/fa.ts
        translations/fi_FI.ts
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
        dependencies/libosmscout/libosmscout-client-qt/src/osmscoutclientqt/RouteDescriptionBuilder.cpp
        dependencies/libosmscout/libosmscout-client-qt/src/osmscoutclientqt/OpeningHoursModel.cpp
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
set_property(TARGET harbour-osmscout PROPERTY CXX_STANDARD 20)
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

install(FILES harbour-osmscout-open-url.desktop
DESTINATION share/applications)

install(FILES cz.karry.osmscout.OSMScout.service
DESTINATION share/dbus-1/services)

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

install(DIRECTORY icons/laneturn/
        DESTINATION share/${HARBOUR_APP_NAME}/laneturn
        FILES_MATCHING PATTERN "*.svg")

install(DIRECTORY icons/map/
        DESTINATION share/${HARBOUR_APP_NAME}/map-icons
        FILES_MATCHING PATTERN "*.svg")

install(DIRECTORY stylesheets/include
        DESTINATION share/${HARBOUR_APP_NAME}/map-styles
        FILES_MATCHING PATTERN "*.oss")

install(FILES
        stylesheets/public-transport.oss
        stylesheets/standard.oss
        stylesheets/outdoor.oss
        stylesheets/winter-sports.oss
        stylesheets/cycle.oss
        DESTINATION share/${HARBOUR_APP_NAME}/map-styles)

# ==================================================================================================
# PerformanceTest binary
set(SOURCE_FILES
    src/PerformanceTest.cpp
)

add_executable(PerformanceTest ${SOURCE_FILES})
set_property(TARGET PerformanceTest PROPERTY CXX_STANDARD 20)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/PerformanceTestConfig.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/PerformanceTest/config.h)

target_include_directories(PerformanceTest PRIVATE 
        ${OSMSCOUT_INCLUDE_DIRS}
        ${LIBSAILFISHAPP_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}/include/PerformanceTest/
)

target_link_libraries(PerformanceTest
        Qt5::Core
        Qt5::Gui
        Qt5::Quick # required by sailfishapp when configured with QT_QML_DEBUG

        marisa
        OSMScout
        OSMScoutMap
        OSMScoutMapQt
        ${LIBSAILFISHAPP_LIBRARIES}
)

# ==================================================================================================
# SearchPerfTest binary

add_executable(SearchPerfTest ${OSMSCOUT_BASE_DIR_SOURCE}/Demos/src/LocationLookup.cpp)
set_property(TARGET SearchPerfTest PROPERTY CXX_STANDARD 20)
target_link_libraries(SearchPerfTest OSMScout)

# ==================================================================================================
# MultiDBRouting

add_executable(MultiDBRouting ${OSMSCOUT_BASE_DIR_SOURCE}/Tests/src/MultiDBRoutingTest.cpp)
set_property(TARGET MultiDBRouting PROPERTY CXX_STANDARD 20)
target_link_libraries(MultiDBRouting OSMScout)

# ==================================================================================================
# Routing

add_executable(Routing ${OSMSCOUT_BASE_DIR_SOURCE}/Demos/src/Routing.cpp)
set_property(TARGET Routing PROPERTY CXX_STANDARD 20)
target_link_libraries(Routing OSMScout)

# ==================================================================================================
# MediaPlayer

add_executable(MediaPlayer src/MediaPlayer.cpp)
set_property(TARGET MediaPlayer PROPERTY CXX_STANDARD 20)
target_include_directories(MediaPlayer PRIVATE
        src
        ${OSMSCOUT_INCLUDE_DIRS}
        ${LIBSAILFISHAPP_INCLUDE_DIRS})

target_link_libraries(MediaPlayer
        Qt5::Core
        Qt5::Gui
        Qt5::Multimedia
        Qt5::Quick

        ${LIBSAILFISHAPP_LIBRARIES}
        )

# ==================================================================================================
# OSTAndOSSCheck

add_executable(OSTAndOSSCheck ${OSMSCOUT_BASE_DIR_SOURCE}/Tests/src/OSTAndOSSTest.cpp)
set_property(TARGET OSTAndOSSCheck PROPERTY CXX_STANDARD 20)
target_link_libraries(OSTAndOSSCheck OSMScout OSMScoutMap)

enable_testing()

set(STYLESHEETS
        standard.oss
        winter-sports.oss
        boundaries.oss
        public-transport.oss
        outdoor.oss
        cycle.oss)

foreach(STYLESHEET ${STYLESHEETS})
    add_test(NAME CheckStyleSheet-${STYLESHEET}
             COMMAND OSTAndOSSCheck
             --warning-as-error
             --poi-type _track
             --poi-type _highlighted
             --poi-type _waypoint
             --poi-type _waypoint_red_circle
             --poi-type _waypoint_green_circle
             --poi-type _waypoint_blue_circle
             --poi-type _waypoint_yellow_circle
             --poi-type _waypoint_red_square
             --poi-type _waypoint_green_square
             --poi-type _waypoint_blue_square
             --poi-type _waypoint_yellow_square
             --poi-type _waypoint_red_triangle
             --poi-type _waypoint_green_triangle
             --poi-type _waypoint_blue_triangle
             --poi-type _waypoint_yellow_triangle
             ${CMAKE_CURRENT_SOURCE_DIR}/stylesheets/map.ost
             ${CMAKE_CURRENT_SOURCE_DIR}/stylesheets/${STYLESHEET})
endforeach()
