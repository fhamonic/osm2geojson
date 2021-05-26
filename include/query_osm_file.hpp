#ifndef QUERY_OSM_FILE_HPP
#define QUERY_OSM_FILE_HPP

#include <filesystem>  // filesystem::path
#include <fstream>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>
#include <osmium/dynamic_handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <osmium/index/map/flex_mem.hpp>

#include "parse_patterns.hpp"
#include "bg_regions_dump_handler.hpp"

std::vector<Region> query_osm_file(
        const std::filesystem::path & input_file,
        const std::filesystem::path & patterns_file, 
        const std::filesystem::path & search_area_pattern_file);

#endif // QUERY_OSM_FILE_HPP