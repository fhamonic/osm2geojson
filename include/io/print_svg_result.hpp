#ifndef PRINT_REGIONS_SVG_HPP
#define PRINT_REGIONS_SVG_HPP

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "bg_types.hpp"
#include "data_types/area.hpp"
#include "data_types/node.hpp"
#include "data_types/way.hpp"

#include <boost/geometry/io/svg/svg_mapper.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

namespace IO {
void print_svg_result(const std::vector<Node> & nodes,
                      const std::vector<Way> & ways,
                      const std::vector<Area> & areas,
                      const std::filesystem::path & svg_file);
}

#endif  // PRINT_REGIONS_SVG_HPP