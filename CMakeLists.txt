cmake_minimum_required(VERSION 3.12)

project(OSM2Landscape VERSION 0.1
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


add_executable(osm2regions src/io/parse_patterns.cpp
                        src/io/print_geojson.cpp
                        src/io/parse_geojson.cpp
                        src/io/print_regions_svg.cpp
                        src/query_osm_file.cpp
                        src/osm2regions.cpp)
target_include_directories(osm2regions PRIVATE include)
target_link_libraries(osm2regions CONAN_PKG::boost)
target_link_libraries(osm2regions CONAN_PKG::simdjson)
target_link_libraries(osm2regions CONAN_PKG::nlohmann_json)
target_link_libraries(osm2regions CONAN_PKG::expat)
target_link_libraries(osm2regions CONAN_PKG::zlib)
target_include_directories(osm2regions PUBLIC ${OSMIUM_INCLUDE_DIR})
target_include_directories(osm2regions PUBLIC ${PROTOZERO_INCLUDE_DIR})
target_compile_options_if_exists(osm2regions PRIVATE ${WARNINGS} -Wall)
target_compile_options_if_exists(osm2regions PRIVATE ${COMPILE_FOR_NATIVE} -march=native)
target_compile_options_if_exists(osm2regions PRIVATE ${COMPILE_WITH_LTO} -flto)


add_executable(regions2landscape src/bg_h3_interface.cpp
                        src/io/parse_geojson.cpp
                        src/io/print_hexagons_svg.cpp
                        src/regions2landscape.cpp)
target_include_directories(regions2landscape PRIVATE include)
target_link_libraries(regions2landscape CONAN_PKG::boost)
target_link_libraries(regions2landscape CONAN_PKG::tbb)
target_link_libraries(regions2landscape CONAN_PKG::simdjson)
target_link_libraries(regions2landscape CONAN_PKG::h3)
target_link_libraries(regions2landscape CONAN_PKG::parallel-hashmap)
target_compile_options_if_exists(regions2landscape PRIVATE ${WARNINGS} -Wall)
target_compile_options_if_exists(regions2landscape PRIVATE ${COMPILE_FOR_NATIVE} -march=native)
target_compile_options_if_exists(regions2landscape PRIVATE ${COMPILE_WITH_LTO} -flto)
