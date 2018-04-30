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
set(OSMSCOUT_SAILFISH_MAJOR_VERSION "0")
set(OSMSCOUT_SAILFISH_MINOR_VERSION "32")
set(OSMSCOUT_SAILFISH_PATCH_VERSION "0")

set(OSMSCOUT_SAILFISH_SUFFIX_VERSION "")
set(OSMSCOUT_SAILFISH_VERSION_SHORT  "${OSMSCOUT_SAILFISH_MAJOR_VERSION}.${OSMSCOUT_SAILFISH_MINOR_VERSION}.${OSMSCOUT_SAILFISH_PATCH_VERSION}")
set(OSMSCOUT_SAILFISH_VERSION_STRING "${OSMSCOUT_SAILFISH_VERSION_SHORT}${OSMSCOUT_SAILFISH_SUFFIX_VERSION}")

# ==================================================================================================

set(CMAKE_MIN_VERSION    "2.8.11")
set(QT_MIN_VERSION       "5.2.0")
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
              Network
              Gui
              Positioning
              Xml
              Svg
              Qml
              Quick
)
find_package(Qt5LinguistTools)
find_package(OpenMP REQUIRED)
#find_package(LibXml2)
#find_package(BZip2)
find_package(SailfishApp) # https://github.com/sailfish-sdk/libsailfishapp 

# ==================================================================================================
# Dependencies Rules
# TODO

# ==================================================================================================

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
message(STATUS "Requiered dependencies:")

PRINT_LIBRARY_STATUS("Qt5"                    "http://www.qtsoftware.com"                       "(version >= ${QT_MIN_VERSION})"     Qt5_FOUND)
#PRINT_LIBRARY_STATUS("libprotobuf"            "https://github.com/google/protobuf"             ""      PROTOBUF_FOUND)
PRINT_LIBRARY_STATUS("OpenMP compiler support" "http://openmp.org/wp/openmp-compilers/"         ""      OPENMP_FOUND)
PRINT_LIBRARY_STATUS("SailfishApp library"     "https://github.com/sailfish-sdk/libsailfishapp" ""      LIBSAILFISHAPP_FOUND)

message(STATUS "")

if(Qt5_FOUND AND
  LIBSAILFISHAPP_FOUND
  )
    message(STATUS      " OSMScout-Sailfish can be compiled........ YES")
    set(OSMSCOUT_SAILFISH_CAN_BE_COMPILED true)
else()
    message(FATAL_ERROR " OSMScout-Sailfish will be compiled....... NO  (Look README file for more details about dependencies)")
    set(OSMSCOUT_SAILFISH_CAN_BE_COMPILED false)
endif()

message(STATUS "----------------------------------------------------------------------------------")
message(STATUS "")

# ==================================================================================================

if (OSMSCOUT_SAILFISH_CAN_BE_COMPILED)

  if (LABEL_LAYOUTER_DEBUG)
    add_definitions( -DLABEL_LAYOUTER_DEBUG)
  endif()
  if (DEBUG_GROUNDTILES)
    add_definitions( -DDEBUG_GROUNDTILES)
  endif()
  if (QT_QML_DEBUG)
    add_definitions( -DQT_QML_DEBUG)
  endif()
  if (SANITIZE)
    # -fsanitize=address
    add_definitions( -fno-omit-frame-pointer -fstack-protector-all)
  endif()

  add_definitions( -Wall -pedantic -fPIC -Wno-unused-function)
  add_definitions(-std=c++11)

  # QT5 definitions
  add_definitions(${QT_DEFINITIONS})
  add_definitions(${Qt5Core_DEFINITIONS})

  add_definitions(${LIBXML2_DEFINITIONS})
  add_definitions(${LIBSAILFISHAPP_DEFINITIONS})

  # OpenMP definitions
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

  set(HARBOUR_APP_NAME harbour-osmscout)
endif()


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

set(OSMSCOUT_BASE_DIR_BUILD ${CMAKE_CURRENT_BINARY_DIR})
set(OSMSCOUT_BASE_DIR_SOURCE ${CMAKE_HOME_DIRECTORY}/dependencies/libosmscout)

set(OSMSCOUT_INCLUDE_DIRS
  ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
  ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
  ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include
  ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-client-qt/include
)

# QT5 definitions
add_definitions(${QT_DEFINITIONS})
add_definitions(${Qt5Core_DEFINITIONS})

# we are including libosmscout subdirectories directly, we are responsible
# to setup all variables correctly
add_subdirectory(dependencies/libosmscout/libosmscout)
add_subdirectory(dependencies/libosmscout/libosmscout-map)
add_subdirectory(dependencies/libosmscout/libosmscout-map-qt)
add_subdirectory(dependencies/libosmscout/libosmscout-client-qt)

# ==================================================================================================
# harbour-osmscout

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
#    src/DBThread.h
)

set(SOURCE_FILES
    src/AppSettings.cpp
    src/OSMScout.cpp

    # keep qml files in source list - it makes qtcreator happy
    # find qml -type f
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
    qml/custom/MapComponent.qml
    qml/custom/ScaleIndicator.qml
    qml/custom/LocationSelector.qml
    qml/custom/RoutingStep.qml
    qml/custom/RouteStepIcon.qml
    qml/custom/Utils.js
    qml/pages/Cover.qml
    qml/pages/Search.qml
    qml/pages/MapList.qml
    qml/pages/Downloads.qml
    qml/pages/MapDetail.qml
    qml/pages/About.qml
    qml/pages/PlaceDetail.qml
    qml/pages/MapPage.qml
    qml/pages/Layers.qml
    qml/pages/MapObjects.qml
    qml/pages/Routing.qml
    qml/pages/RouteDescription.qml
    qml/main.qml
    qml/l10n.qml
    qml/desktop.qml
)

# XML files with translated phrases.
# You can add new language translation just by adding new entry here, and run build.
# Newly created *.ts file may be edited by linguist tool.
# Please don't forget to add new *.ts file to Meson-based build.
set(TRANSLATION_TS_FILES
        translations/en.ts

        translations/cs.ts
        translations/de.ts
        translations/es.ts
        translations/fr.ts
        translations/hu.ts
        translations/nb.ts
        translations/nl.ts
        translations/pl.ts
        translations/sv.ts
        )

# source files that may contains translatable phrases (see tr, qsTr, QT_TRANSLATE_NOOP functions)
set(TRANSLATION_SOURCE_FILES
        ${SOURCE_FILES}
        src/OSMScout.cpp
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

add_executable(harbour-osmscout ${SOURCE_FILES} ${HEADER_FILES})
add_dependencies(harbour-osmscout translations)

# Private config header
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CMakeMod/Config.h.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/privateinclude/harbour-osmscout/private/Config.h")

target_include_directories(harbour-osmscout PRIVATE 
        ${CMAKE_CURRENT_BINARY_DIR}/privateinclude
        ${OSMSCOUT_INCLUDE_DIRS}
        ${LIBSAILFISHAPP_INCLUDE_DIRS}
)

target_link_libraries(harbour-osmscout
        Qt5::Core
        Qt5::Gui
        Qt5::Qml
        Qt5::Quick
        Qt5::Svg
        Qt5::Positioning
        
        marisa
        OSMScout
        OSMScoutMap
        OSMScoutMapQt
        OSMScoutClientQt
        ${LIBSAILFISHAPP_LIBRARIES}
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

install(DIRECTORY icons/poi/
        DESTINATION share/${HARBOUR_APP_NAME}/poi-icons
        FILES_MATCHING PATTERN "*.svg")

install(DIRECTORY icons/routestep/
        DESTINATION share/${HARBOUR_APP_NAME}/routestep
        FILES_MATCHING PATTERN "*.svg")

install(FILES
  dependencies/libosmscout/libosmscout/data/icons/14x14/standard/bus_stop.png
  dependencies/libosmscout/libosmscout/data/icons/14x14/standard/cemetery.png
  dependencies/libosmscout/libosmscout/data/icons/14x14/standard/forest.png
  dependencies/libosmscout/libosmscout/data/icons/14x14/standard/hospital.png
  dependencies/libosmscout/libosmscout/data/icons/14x14/standard/parking.png
  dependencies/libosmscout/libosmscout/data/icons/14x14/standard/pharmacy.png
  dependencies/libosmscout/libosmscout/data/icons/14x14/standard/scrub.png
DESTINATION share/${HARBOUR_APP_NAME}/map-icons)

install(FILES 
  stylesheets/boundaries.oss
  stylesheets/standard.oss
  stylesheets/winter-sports.oss
  stylesheets/public-transport.oss
DESTINATION share/${HARBOUR_APP_NAME}/map-styles)

# ==================================================================================================
# PerformanceTest binary
set(SOURCE_FILES
    src/PerformanceTest.cpp
)

add_executable(PerformanceTest ${SOURCE_FILES})

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
set_property(TARGET MultiDBRouting PROPERTY CXX_STANDARD 11)
target_include_directories(MultiDBRouting PRIVATE ${OSMSCOUT_INCLUDE_DIRS})
target_link_libraries(MultiDBRouting
        OSMScout
        )

# ==================================================================================================
# Routing

add_executable(Routing ${OSMSCOUT_BASE_DIR_SOURCE}/Demos/src/Routing.cpp)
set_property(TARGET Routing PROPERTY CXX_STANDARD 11)
target_include_directories(Routing PRIVATE ${OSMSCOUT_INCLUDE_DIRS})
target_link_libraries(Routing
        OSMScout
        )
