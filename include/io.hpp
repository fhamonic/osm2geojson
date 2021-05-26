#ifndef IO_HPP
#define IO_HPP 

#include <filesystem>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <limits>

#include "bg_types.hpp"
#include "parse_patterns.hpp"

#include <boost/geometry/io/svg/svg_mapper.hpp>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>

void plot_svg(const std::vector<Region> & regions, const std::filesystem::path & svg_file);
void print_geojson(const std::vector<Region> & regions, const std::filesystem::path & json_file);
std::vector<Region> parse_geojson(const std::filesystem::path & json_file);

#endif // IO_HPP