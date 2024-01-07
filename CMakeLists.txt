cmake_minimum_required(VERSION 3.12)

project(OSM2GeoJSON VERSION 0.1
    DESCRIPTION "Program to handle openstreetmap datas and retrieve informations about ecological landscapes"
    LANGUAGES CXX)

# set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

#################### Options ####################
SET(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WARNINGS "Enable warnings" OFF)
option(OPTIMIZE_FOR_NATIVE "Build with -march=native" OFF)

# ################### Modules ####################
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(GNUInstallDirs)
include(CompilerWarnings)
include(CompilerOptimizations)

#################### Packages ####################
find_package(nlohmann_json REQUIRED)
find_package(simdjson REQUIRED)
# find_package(nlohmann_json_schema_validator REQUIRED)
# find_package(phmap REQUIRED)
find_package(Boost REQUIRED)
find_package(TBB REQUIRED)

set(Osmium_DEBUG 1)
# set(Osmium_FIND_COMPONENTS 1)
# find_package(Osmium REQUIRED)
find_package(Osmium REQUIRED COMPONENTS xml pbf io)
find_package(expat REQUIRED)

add_executable(osm2geojson src/io/parse_patterns.cpp
                        src/io/print_geojson.cpp
                        src/io/print_svg_result.cpp
                        src/query_osm_file.cpp
                        src/osm2geojson.cpp)
target_include_directories(osm2geojson PRIVATE include)
target_link_libraries(osm2geojson nlohmann_json::nlohmann_json)
target_link_libraries(osm2geojson simdjson::simdjson)
target_link_libraries(osm2geojson Boost::boost)
target_link_libraries(osm2geojson Boost::program_options Boost::log Boost::log_setup)
target_link_libraries(osm2geojson TBB::tbb)
target_include_directories(osm2geojson PUBLIC ${OSMIUM_INCLUDE_DIR})
target_link_libraries(osm2geojson ${OSMIUM_LIBRARIES})
target_link_libraries(osm2geojson expat::expat)

set_project_warnings(osm2geojson)
set_project_optimizations(osm2geojson)
