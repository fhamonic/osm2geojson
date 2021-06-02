#ifndef QUERY_OSM_FILE_HPP
#define QUERY_OSM_FILE_HPP

#include <filesystem>  // filesystem::path
#include <fstream>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>
#include <osmium/dynamic_handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <osmium/index/map/flex_mem.hpp>

#include "io/parse_patterns.hpp"
#include "bg_regions_dump_handler.hpp"

MultipolygonGeo query_osm_search_area(const std::filesystem::path & input_file,
        const std::filesystem::path & search_area_pattern_file);

std::vector<Region> query_osm_regions(const std::filesystem::path & input_file,
        const std::filesystem::path & patterns_file, 
        const MultipolygonGeo & search_area);

#endif // QUERY_OSM_FILE_HPP