#ifndef PRINT_HEXAGONS_SVG_HPP
#define PRINT_HEXAGONS_SVG_HPP 

#include <filesystem>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "bg_types.hpp"
#include "bg_h3_interface.hpp"

#include <boost/geometry/io/svg/svg_mapper.hpp>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>

namespace IO {
    void print_hexagons_svg(std::vector<std::tuple<H3Index, double, double>> hex_datas, const std::filesystem::path & svg_file, const BoxGeo & bbox);
    void print_hexagons_svg(std::vector<std::tuple<H3Index, double, double>> hex_datas, const std::filesystem::path & svg_file);
}

#endif // PRINT_HEXAGONS_SVG_HPP