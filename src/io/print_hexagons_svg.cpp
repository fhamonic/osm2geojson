
#include "io/print_hexagons_svg.hpp"

namespace bg = boost::geometry;
namespace ba = boost::adaptors;

namespace IO {
    void print_hexagons_svg(std::vector<std::tuple<H3Index, double, double>> hex_datas, const std::filesystem::path & svg_file, const BoxGeo & bbox) {
        std::ofstream svg(svg_file);
        bg::svg_mapper<PointGeo> mapper(svg, 100,100);
        mapper.add(bbox);

        double min_quality = *boost::min_element(hex_datas | ba::transformed([](auto & t){ return std::get<1>(t); }));
        double max_quality = *boost::max_element(hex_datas | ba::transformed([](auto & t){ return std::get<1>(t); }));
        double min_resistance = *boost::min_element(hex_datas | ba::transformed([](auto & t){ return std::get<2>(t); }));
        double max_resistance = *boost::max_element(hex_datas | ba::transformed([](auto & t){ return std::get<2>(t); }));

        for(const auto & [i, quality, prob] : hex_datas) {
            RingGeo r = indexToRing(i);
            int red = 255 - 255 * ((max_resistance-min_resistance)!=0 ? (prob-min_resistance)/(max_resistance-min_resistance) : 1);
            int green = 255 * ((max_quality-min_quality)!=0 ? (quality-min_quality)/(max_quality-min_quality) : 1);
            mapper.map(r, "fill-opacity:0.5;fill:rgb(" + std::to_string(red) + "," + std::to_string(green) + ",0);stroke:rgb(0,0,0);stroke-width:0");
        }
    }

    void print_hexagons_svg(std::vector<std::tuple<H3Index, double, double>> hex_datas, const std::filesystem::path & svg_file) {
        BoxGeo bbox = bg::make_inverse<BoxGeo>();
        for(const auto & [i, quality, prob] : hex_datas) {
            bg::expand(bbox, bg::return_envelope<BoxGeo>(indexToRing(i)));
        }
        print_hexagons_svg(hex_datas, svg_file, bbox);
    }
}