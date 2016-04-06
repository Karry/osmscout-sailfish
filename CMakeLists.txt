#
# Copyright (c) 2016 Lukas Karas <lukas.karas@centrum.cz>
#
# Structure of MoNav cmake scripts was inspired by DigiKam project
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

project(osmscout-sailfish C CXX)

# ==================================================================================================
# Information to update before to release this package.

# project version
set(OSMSCOUT_SAILFISH_MAJOR_VERSION "0")
set(OSMSCOUT_SAILFISH_MINOR_VERSION "1")
set(OSMSCOUT_SAILFISH_PATCH_VERSION "0")

set(OSMSCOUT_SAILFISH_SUFFIX_VERSION "-alpha1")
set(OSMSCOUT_SAILFISH_VERSION_SHORT  "${OSMSCOUT_SAILFISH_MAJOR_VERSION}.${OSMSCOUT_SAILFISH_MINOR_VERSION}.${OSMSCOUT_SAILFISH_PATCH_VERSION}")
set(OSMSCOUT_SAILFISH_VERSION_STRING "${OSMSCOUT_SAILFISH_VERSION_SHORT}${OSMSCOUT_SAILFISH_SUFFIX_VERSION}")

# ==================================================================================================

set(CMAKE_MIN_VERSION    "2.8.12")
set(QT_MIN_VERSION       "5.2.0")

cmake_minimum_required(VERSION ${CMAKE_MIN_VERSION})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_HOME_DIRECTORY}/CMakeMod")

# Cmake macros
include(FeatureSummary)

# Local macros
include(MacroUtils)

# feature detection (libraries, header, etc.)
#include(CheckCXXCompilerFlag)
#include(features)
function(create_private_config output name)
  string(REPLACE "-" "_" OSMSCOUT_PRIVATE_CONFIG_HEADER_NAME ${name})
  string(TOUPPER ${OSMSCOUT_PRIVATE_CONFIG_HEADER_NAME} OSMSCOUT_PRIVATE_CONFIG_HEADER_NAME)
  configure_file("${OSMSCOUT_BASE_DIR_SOURCE}/cmake/Config.h.cmake" ${output})
endfunction()


# ==================================================================================================
# Find dependencies

find_package( Qt5 ${QT_MIN_VERSION} 
              REQUIRED NO_MODULE COMPONENTS 
              Core
              Network
              Widgets
              Gui
              Positioning
              Xml
              Svg
              Qml
              Quick
)
find_package(OpenMP REQUIRED)
find_package(LibXml2)
find_package(BZip2)

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

PRINT_LIBRARY_STATUS("Qt5"                    "http://www.qtsoftware.com"             "(version >= ${QT_MIN_VERSION})"     Qt5_FOUND)
#PRINT_LIBRARY_STATUS("libprotobuf"            "https://github.com/google/protobuf"    ""      PROTOBUF_FOUND)
PRINT_LIBRARY_STATUS("OpenMP compiler support" "http://openmp.org/wp/openmp-compilers/" ""    OPENMP_FOUND)

message(STATUS "")

if(Qt5_FOUND   
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
# libmarisa

# marisa is pure C code
add_definitions( -std=c11 )

set(MARISA_ROOT ${CMAKE_HOME_DIRECTORY}/dependencies/marisa-0.2.4)

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
    #${MARISA_ROOT}/tools/marisa-common-prefix-search.cc
    #${MARISA_ROOT}/tools/marisa-benchmark.cc
    #${MARISA_ROOT}/tools/marisa-predictive-search.cc
    #${MARISA_ROOT}/tools/marisa-build.cc
    #${MARISA_ROOT}/tools/marisa-reverse-lookup.cc
    #${MARISA_ROOT}/tools/marisa-lookup.cc
    #${MARISA_ROOT}/tools/marisa-dump.cc
    #${MARISA_ROOT}/tools/cmdopt.cc
)

add_library(marisa SHARED 
    ${MARISA_SRC}
)

set(MARISA_FOUND TRUE)
set(MARISA_LIBRARIES marisa)

# ==================================================================================================
set(OSMSCOUT_BASE_DIR_BUILD ${CMAKE_CURRENT_BINARY_DIR})
set(OSMSCOUT_BASE_DIR_SOURCE ${CMAKE_HOME_DIRECTORY}/dependencies/libosmscout)

include_directories(
  ${OSMSCOUT_BASE_DIR_BUILD}/include/
)

# following code is c++
remove_definitions(-std=c11)
add_definitions(-std=c++11)

add_definitions( -Wall -pedantic -fPIC -Wno-unused-function)

# QT5 definitions
add_definitions(${QT_DEFINITIONS})
add_definitions(${Qt5Core_DEFINITIONS})


add_subdirectory(dependencies/libosmscout/libosmscout)
add_subdirectory(dependencies/libosmscout/libosmscout-map)
add_subdirectory(dependencies/libosmscout/libosmscout-map-qt)

# ==================================================================================================
# osmscout-sailfish

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
    src/DBThread.h
    src/MapWidget.h
    src/RoutingModel.h
    src/SearchLocationModel.h
    src/Settings.h
    src/Theme.h
)

set(SOURCE_FILES
    src/DBThread.cpp
    src/MapWidget.cpp
    src/OSMScout.cpp
    src/RoutingModel.cpp
    src/SearchLocationModel.cpp
    src/Settings.cpp
    src/Theme.cpp
)

set(RESOURCE_FILES
    res.qrc
)

add_executable(osmscout-sailfish ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})

target_include_directories(osmscout-sailfish PRIVATE 
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include)

target_link_libraries(osmscout-sailfish
        libosmscout
        libosmscout_map
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Qml
        Qt5::Quick
        Qt5::Svg
        Qt5::Positioning
        libosmscout_map_qt)

set_property(TARGET osmscout-sailfish PROPERTY CXX_STANDARD 11)
