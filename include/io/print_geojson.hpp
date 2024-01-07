#ifndef PRINT_GEOJSON_HPP
#define PRINT_GEOJSON_HPP

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "bg_types.hpp"
#include "data_types/area.hpp"
#include "data_types/node.hpp"
#include "data_types/way.hpp"

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/algorithm/string.hpp>

namespace IO {
void print_geojson(const std::vector<Node> & nodes,
                   const std::vector<Way> & ways,
                   const std::vector<Area> & areas,
                   const std::filesystem::path & json_file);
}
#endif  // PRINT_GEOJSON_HPP