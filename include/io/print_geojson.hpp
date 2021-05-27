#ifndef PRINT_GEOJSON_HPP
#define PRINT_GEOJSON_HPP 

#include <filesystem>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "bg_types.hpp"
#include "region.hpp"

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>

namespace IO {
    void print_geojson(const std::vector<Region> & regions, const std::filesystem::path & json_file);
}
#endif // PRINT_GEOJSON_HPP