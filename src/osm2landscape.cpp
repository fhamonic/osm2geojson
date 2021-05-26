#include <cstdlib>  // std::exit
#include <execution> // std::execution::par
#include <filesystem>  // filesystem::path
#include <fstream>  // ofstream
#include <iostream> // std::cout, std::cerr

#include <osmium/io/file.hpp>

#include <nlohmann/json.hpp>

#include "query_osm_file.hpp"
#include "bg_utils.hpp"



#include <utility>
#include <vector>

namespace bg = boost::geometry;

using PointGeo = bg::model::d2::point_xy<double, bg::cs::geographic<bg::degree>>;
using BoxGeo = bg::model::box<PointGeo>;
using LinestringGeo = bg::model::linestring<PointGeo>;
using PolygonGeo = bg::model::polygon<PointGeo>;
using MultipolygonGeo = bg::model::multi_polygon<PolygonGeo>;


#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/index/detail/predicates.hpp>
namespace bgi = bg::index;

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>
namespace ba = boost::adaptors;

#include "bg_h3_interface.hpp"


#define DEFAULT_QUALITY_COEF 0
#define DEFAULT_PROB_COEF 0

#include "chrono.hpp"

void print_usage(const char* prgname) {
    std::cerr << "Usage: " << prgname << " PATTERNSFILE OSMFILE HEX_RESOLUTION\n";
    std::exit(1);
}

int main(int argc, char* argv[]) {
    if (argc != 4) print_usage(argv[0]);
    std::filesystem::path patterns_file{argv[1]};
    osmium::io::File osm_file{argv[2]};
    int hex_resolution{std::atoi(argv[3])};

    Chrono chrono;
    std::cout << "Start to parse OSM datas" << std::endl;

    std::ifstream patterns_stream(patterns_file);
    nlohmann::json patterns_json;
    patterns_stream >> patterns_json;

    BGRegionsDumpHandler<RegionInfo,RegionInfo,RegionInfo> bg_handler = query_osm_file(osm_file, patterns_json);

    std::vector<std::pair<MultipolygonGeo,RegionInfo>> regions;
    regions.reserve(bg_handler.getNodes().size() + bg_handler.getWays().size() + bg_handler.getAreas().size());

    boost::transform(bg_handler.getNodes(), std::back_inserter(regions),
        [&](auto & p) { return std::make_pair(buffer_PointGeo(p.first, p.second.inflate), p.second); });
    boost::transform(bg_handler.getWays(), std::back_inserter(regions),
        [&](auto & p) { return std::make_pair(buffer_LinestringGeo(p.first, p.second.inflate), p.second); });
    boost::copy(bg_handler.getAreas(), std::back_inserter(regions));


    for(const auto & r : regions) {
        if(r.first.empty()) {
            std::cerr << "caca empty" << std::endl;
            return EXIT_FAILURE;
        }
        if(!bg::is_valid(r.first)) {
            std::cerr << "caca invalid" << std::endl;
            return EXIT_FAILURE;
        }
    }

    plot_svg(regions, "my_map.svg");

    std::cout << "Compelte parsing OSM datas in " << chrono.lapTimeMs() << " ms" << std::endl;
    std::cout << "Start to polyfill" << std::endl;


    using RTree = bgi::rtree<std::pair<BoxGeo,size_t>, bgi::rstar<16,4>>;
    RTree rtree(regions | ba::indexed(0) | ba::transformed([](const auto& e) {
        return std::make_pair(bg::return_envelope<BoxGeo>(e.value().first), e.index());
    }));

    const MultipolygonGeo & search_area = bg_handler.getSearchArea();
    PolygonGeo hull;
    bg::convex_hull(search_area, hull);

    std::vector<H3Index> hex_indices = polyfill(hull, hex_resolution);
    std::vector<std::tuple<H3Index, double, double>> hex_datas;
    hex_datas.resize(hex_indices.size());

    // int caca_diff = 0;

    std::transform(std::execution::par_unseq, hex_indices.cbegin(), hex_indices.cend(), hex_datas.begin(), [&rtree, &regions/*, &caca_diff*/](const H3Index i){ 
        RingGeo r = indexToRing(i);
        PointGeo center = indexToCenter(i);
        srs::projection<> proj = srs::proj4("+proj=eqc +ellps=GRS80 +lon_0="+ std::to_string(center.get<0>()) +" +lat_0="+ std::to_string(center.get<1>()));
        Ring2D r2d;
        proj.forward(r, r2d);

        std::vector<std::pair<Multipolygon2D, RegionInfo>> intersected_regions2d;
        for(const auto & result : rtree | bgi::adaptors::queried(bgi::intersects(bg::return_envelope<BoxGeo>(r)) && bgi::satisfies([&regions, &r](const auto & v) {
                return bg::intersects(r, regions[v.second].first);}))) {
            intersected_regions2d.emplace_back(std::piecewise_construct, 
                    std::forward_as_tuple(), std::forward_as_tuple(regions[result.second].second));
            proj.forward(regions[result.second].first, intersected_regions2d.back().first);
        }

        boost::sort(intersected_regions2d, [](std::pair<Multipolygon2D, RegionInfo> e1, std::pair<Multipolygon2D, RegionInfo> e2){
            if(e1.second.overrideLevel == e2.second.overrideLevel)
                return e1.second.qualityCoef > e2.second.qualityCoef;
            return e1.second.overrideLevel > e2.second.overrideLevel;
        });

        Multipolygon2D mp2d;
        bg::convert(r2d, mp2d);

        const double area = bg::area(r2d);
        double sum_quality = 0;
        double sum_prob = 0;
        for(const auto & p : intersected_regions2d) {
            Multipolygon2D intersections;
            bg::intersection(r2d, p.first, intersections);

            const double intersections_area = bg::area(intersections);
            sum_quality += intersections_area * p.second.qualityCoef;
            sum_prob += intersections_area * p.second.probConnectionPerMeter;
            Multipolygon2D diff_result;
            bg::difference(mp2d, intersections, diff_result);
            mp2d = std::move(diff_result);

            if(bg::is_empty(mp2d)) break;
            // if(!bg::is_valid(mp2d)) ++caca_diff;
        }
        sum_quality += DEFAULT_QUALITY_COEF * bg::area(mp2d);
        sum_prob += DEFAULT_PROB_COEF * bg::area(mp2d);

        return std::make_tuple(i, sum_quality/area, sum_prob/area);
    });

    // std::cout << "CACA:" << std::endl
    //         << "\tdiff: " << caca_diff << std::endl ;

    std::cout << "Compelte polyfill in " << chrono.lapTimeMs() << " ms" << std::endl;



    std::ofstream svg("marseille_weighted_hexs.svg");
    bg::svg_mapper<PointGeo> mapper(svg, 100,100);
    mapper.add(hull);
    mapper.map(hull, "fill-opacity:0.2;fill:rgb(32,32,32);stroke:rgb(0,0,0);stroke-width:0.01");

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