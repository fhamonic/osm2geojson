cmake_minimum_required(VERSION 3.12)

project(OSM2Sorted_GeoJSON VERSION 0.1
    DESCRIPTION "Program to handle openstreetmap datas and retrieve informations about ecological landscapes"
    LANGUAGES CXX)

# set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

#################### Options ####################
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeBuildTypeUtils.cmake)

option(WARNINGS "" OFF)
option(COMPILE_FOR_NATIVE "" OFF)
option(COMPILE_WITH_LTO "" OFF)

#################### Packages ####################
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Osmium REQUIRED)
find_package(Protozero REQUIRED)


add_executable(osm2sorted_geojson src/io/parse_patterns.cpp
                        src/io/parse_geojson.cpp
                        src/io/print_geojson.cpp
                        src/io/print_svg_regions.cpp
                        src/query_osm_file.cpp
                        src/osm2sorted_geojson.cpp)
target_include_directories(osm2sorted_geojson PRIVATE include)
target_link_libraries(osm2sorted_geojson CONAN_PKG::boost)
target_link_libraries(osm2sorted_geojson CONAN_PKG::simdjson)
target_link_libraries(osm2sorted_geojson CONAN_PKG::nlohmann_json)
target_link_libraries(osm2sorted_geojson CONAN_PKG::expat)
target_link_libraries(osm2sorted_geojson CONAN_PKG::zlib)
target_link_libraries(osm2sorted_geojson CONAN_PKG::fmt)
target_include_directories(osm2sorted_geojson PUBLIC ${OSMIUM_INCLUDE_DIR})
target_include_directories(osm2sorted_geojson PUBLIC ${PROTOZERO_INCLUDE_DIR})
target_compile_options_if_exists(osm2sorted_geojson PRIVATE ${WARNINGS} -Wall)
target_compile_options_if_exists(osm2sorted_geojson PRIVATE ${COMPILE_FOR_NATIVE} -march=native)
target_compile_options_if_exists(osm2sorted_geojson PRIVATE ${COMPILE_WITH_LTO} -flto)
