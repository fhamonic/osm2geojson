#ifndef PRINT_SVG_HPP
#define PRINT_SVG_HPP 

#include <filesystem>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "bg_types.hpp"
#include "region.hpp"

#include <boost/geometry/io/svg/svg_mapper.hpp>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>

namespace IO {
    void print_svg(const std::vector<Region> & regions, const std::filesystem::path & svg_file);
}

#endif // PRINT_SVG_HPP