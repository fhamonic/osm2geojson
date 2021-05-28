#ifndef PARSE_GEOJSON_HPP
#define PARSE_GEOJSON_HPP 

#include <filesystem>
#include <iostream>

#include "bg_types.hpp"
#include "region.hpp"

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>
#include <simdjson.h>

namespace IO {
    std::vector<Region> parse_geojson(const std::filesystem::path & json_file);
    MultipolygonGeo parse_geojson_multipolygon(const std::filesystem::path & json_file);
}
#endif // PARSE_GEOJSON_HPP