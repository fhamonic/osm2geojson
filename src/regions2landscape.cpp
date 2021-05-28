#include <execution> // std::execution::par
#include <filesystem>  // filesystem::path
#include <fstream>  // ofstream
#include <iostream> // std::cout, std::cerr

#include "bg_types.hpp"
namespace bg = boost::geometry;
#include <boost/geometry/srs/epsg.hpp>
#include <boost/geometry/srs/projection.hpp>
namespace srs = bg::srs;
#include <boost/geometry/index/rtree.hpp>
namespace bgi = boost::geometry::index;
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>
namespace ba = boost::adaptors;
#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

#include "region.hpp"
#include "io/parse_geojson.hpp"
#include "io/print_svg.hpp"
#include "bg_h3_interface.hpp"

#include "chrono.hpp"

static bool process_command_line(int argc, char* argv[], 
    std::filesystem::path & input_file, std::filesystem::path & output_file,
    int & hexagons_resolution, bool & generate_svg) {
    try {
        bpo::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("input,i", bpo::value<std::filesystem::path>(&input_file)->required(), "set input geojson file")
            ("output,o", bpo::value<std::filesystem::path>(&output_file)->required(), "set output geojson file")
            ("hexagons-level,l", bpo::value<int>(&hexagons_resolution)->required(), "set regions patterns description json file")
            ("generate-svg", "generate the svg file of the result regions")
        ;
        bpo::positional_options_description p;
        p.add("input", 1).add("output", 1).add("hexagons-level", 1);
        bpo::variables_map vm;
        bpo::store(bpo::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
        if(vm.count("help")) {
            std::cout << desc << "\n";
            return false;
        }
        bpo::notify(vm); 
        generate_svg = (vm.count("generate-svg") > 0);
    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::filesystem::path input_file;
    std::filesystem::path output_file;
    int hex_resolution;
    bool generate_svg;

    bool valid_command = process_command_line(argc, argv, input_file, output_file, hex_resolution, generate_svg);
    if(!valid_command)
        return EXIT_FAILURE;

    Chrono chrono;

    std::vector<Region> raw_regions = IO::parse_geojson(input_file);
    
    std::cout << "parse regions in " << chrono.lapTimeMs() << " ms" << std::endl;
    
    if(!std::all_of(raw_regions.cbegin(), raw_regions.cend(), [](const auto & p){ return p.hasProperty("qualityCoef") && p.hasProperty("probConnectionPerMeter"); }))
        throw std::runtime_error("require \"qualityCoef\" and \"probConnectionPerMeter\" properties for every regions");

    std::vector<std::pair<MultipolygonGeo,std::pair<double, double>>> regions(raw_regions.size());
    try {
        std::transform(raw_regions.cbegin(), raw_regions.cend(), regions.begin(), [](const Region & r){ 
            return std::make_pair(r.multipolygon, std::make_pair(std::atof(r.getProperty("qualityCoef").c_str()), 
                        std::atof(r.getProperty("probConnectionPerMeter").c_str()))); });
    } catch(std::invalid_argument & e) {
        throw std::runtime_error("require \"qualityCoef\" and \"probConnectionPerMeter\" properties to be numbers");
    }
    
    std::cout << "raw_regions to regions in " << chrono.lapTimeMs() << " ms" << std::endl;

    using RTree = bgi::rtree<std::pair<BoxGeo,size_t>, bgi::rstar<16,4>>;
    RTree rtree(regions | ba::indexed(0) | ba::transformed([](const auto& e) {
        return std::make_pair(bg::return_envelope<BoxGeo>(e.value().first), e.index());
    }));


    std::cout << "construct R-tree in " << chrono.lapTimeMs() << " ms" << std::endl;

    // PolygonGeo hull;
    // // if(search_area_provided) {
    // //     MultipolygonGeo search_area = bg_handler.getSearchArea();
    // //     bg::convex_hull(search_area, hull);
    // // } else {
    //     bg::convert(rtree.bounds(), hull);
    // // }



    // std::vector<H3Index> hex_indices = polyfill(hull, hex_resolution);
    // std::vector<std::tuple<H3Index, double, double>> hex_datas;
    // hex_datas.resize(hex_indices.size());

    // // int caca_diff = 0;

    // std::transform(std::execution::par_unseq, hex_indices.cbegin(), hex_indices.cend(), hex_datas.begin(), [&rtree, &regions/*, &caca_diff*/](const H3Index i){ 
    //     RingGeo r = indexToRing(i);
    //     PointGeo center = indexToCenter(i);
    //     srs::projection<> proj = srs::proj4("+proj=eqc +ellps=GRS80 +lon_0="+ std::to_string(center.get<0>()) +" +lat_0="+ std::to_string(center.get<1>()));
    //     Ring2D r2d;
    //     proj.forward(r, r2d);

    //     std::vector<std::pair<Multipolygon2D, RegionInfo>> intersected_regions2d;
    //     for(const auto & result : rtree | bgi::adaptors::queried(bgi::intersects(bg::return_envelope<BoxGeo>(r)) && bgi::satisfies([&regions, &r](const auto & v) {
    //             return bg::intersects(r, regions[v.second].first);}))) {
    //         intersected_regions2d.emplace_back(std::piecewise_construct, 
    //                 std::forward_as_tuple(), std::forward_as_tuple(regions[result.second].second));
    //         proj.forward(regions[result.second].first, intersected_regions2d.back().first);
    //     }

    //     boost::sort(intersected_regions2d, [](std::pair<Multipolygon2D, RegionInfo> e1, std::pair<Multipolygon2D, RegionInfo> e2){
    //         if(e1.second.overrideLevel == e2.second.overrideLevel)
    //             return e1.second.qualityCoef > e2.second.qualityCoef;
    //         return e1.second.overrideLevel > e2.second.overrideLevel;
    //     });

    //     Multipolygon2D mp2d;
    //     bg::convert(r2d, mp2d);

    //     const double area = bg::area(r2d);
    //     double sum_quality = 0;
    //     double sum_prob = 0;
    //     for(const auto & p : intersected_regions2d) {
    //         Multipolygon2D intersections;
    //         bg::intersection(r2d, p.first, intersections);

    //         const double intersections_area = bg::area(intersections);
    //         sum_quality += intersections_area * p.second.qualityCoef;
    //         sum_prob += intersections_area * p.second.probConnectionPerMeter;
    //         Multipolygon2D diff_result;
    //         bg::difference(mp2d, intersections, diff_result);
    //         mp2d = std::move(diff_result);

    //         if(bg::is_empty(mp2d)) break;
    //         // if(!bg::is_valid(mp2d)) ++caca_diff;
    //     }
    //     sum_quality += DEFAULT_QUALITY_COEF * bg::area(mp2d);
    //     sum_prob += DEFAULT_PROB_COEF * bg::area(mp2d);

    //     return std::make_tuple(i, sum_quality/area, sum_prob/area);
    // });

    // // std::cout << "CACA:" << std::endl
    // //         << "\tdiff: " << caca_diff << std::endl ;

    // std::cout << "Compelte polyfill in " << chrono.lapTimeMs() << " ms" << std::endl;



    // std::ofstream svg("marseille_weighted_hexs.svg");
    // bg::svg_mapper<PointGeo> mapper(svg, 100,100);
    // mapper.add(hull);
    // mapper.map(hull, "fill-opacity:0.2;fill:rgb(32,32,32);stroke:rgb(0,0,0);stroke-width:0.01");

    // double min_quality = *boost::min_element(hex_datas | ba::transformed([](auto & t){ return std::get<1>(t); }));
    // double max_quality = *boost::max_element(hex_datas | ba::transformed([](auto & t){ return std::get<1>(t); }));
    // double min_resistance = *boost::min_element(hex_datas | ba::transformed([](auto & t){ return std::get<2>(t); }));
    // double max_resistance = *boost::max_element(hex_datas | ba::transformed([](auto & t){ return std::get<2>(t); }));

    // for(const auto & [i, quality, prob] : hex_datas) {
    //     RingGeo r = indexToRing(i);
    //     int red = 255 - 255 * ((max_resistance-min_resistance)!=0 ? (prob-min_resistance)/(max_resistance-min_resistance) : 1);
    //     int green = 255 * ((max_quality-min_quality)!=0 ? (quality-min_quality)/(max_quality-min_quality) : 1);
    //     mapper.map(r, "fill-opacity:0.5;fill:rgb(" + std::to_string(red) + "," + std::to_string(green) + ",0);stroke:rgb(0,0,0);stroke-width:0");
    // }
}