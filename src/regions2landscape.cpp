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
#include "io/print_hexagons_svg.hpp"
#include "bg_h3_interface.hpp"

#include "chrono.hpp"

#define DEFAULT_QUALITY_COEF 0
#define DEFAULT_PROB_COEF 0.995

struct RegionInfos {
    double qualityCoef;
    double probConnectionPerMeter;
    int overrideLevel;

    public:
        RegionInfos() = default;
        RegionInfos(double qualityCoef, double probConnectionPerMeter, int overrideLevel)
            : qualityCoef(qualityCoef)
            , probConnectionPerMeter(probConnectionPerMeter)
            , overrideLevel(overrideLevel) {}
};

static bool process_command_line(int argc, char* argv[], 
    std::filesystem::path & input_file, std::filesystem::path & output_file, std::filesystem::path & area_file,
    int & hexagons_resolution, bool & generate_svg, bool & area_provided) {
    try {
        bpo::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("input,i", bpo::value<std::filesystem::path>(&input_file)->required(), "set input geojson file")
            ("output,o", bpo::value<std::filesystem::path>(&output_file)->required(), "set output geojson file")
            ("area,a", bpo::value<std::filesystem::path>(&area_file), "set area to process geojson geometry file, if not precised the whole box area is processed")
            ("hexagons-level,l", bpo::value<int>(&hexagons_resolution)->required(), "set h3 hexagons resolution")
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
        area_provided = (vm.count("area") > 0);
    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::filesystem::path input_file;
    std::filesystem::path output_file;
    std::filesystem::path area_file;
    int hex_resolution;
    bool generate_svg;
    bool area_provided;

    bool valid_command = process_command_line(argc, argv, input_file, output_file, area_file, hex_resolution, generate_svg, area_provided);
    if(!valid_command)
        return EXIT_FAILURE;

    Chrono chrono;

    std::vector<Region> raw_regions = IO::parse_geojson(input_file);
    
    std::cout << "parse regions in " << chrono.lapTimeMs() << " ms" << std::endl;
    
    std::vector<std::pair<MultipolygonGeo,RegionInfos>> regions(raw_regions.size());
    try {
        std::transform(raw_regions.cbegin(), raw_regions.cend(), regions.begin(), [](const Region & r) {
            if(! (r.hasProperty("qualityCoef") && r.hasProperty("probConnectionPerMeter")))
                throw std::runtime_error("require \"qualityCoef\" and \"probConnectionPerMeter\" properties for every regions");
            return std::make_pair(r.multipolygon, RegionInfos(
                std::atof(r.getProperty("qualityCoef").c_str()), 
                std::atof(r.getProperty("probConnectionPerMeter").c_str()), 
                r.hasProperty("overrideLevel") ? std::atoi(r.getProperty("overrideLevel").c_str()) : 0));
        });
    } catch(std::invalid_argument & e) {
        throw std::runtime_error("require \"qualityCoef\" and \"probConnectionPerMeter\" properties to be number strings");
    }
    
    std::cout << "raw_regions to regions in " << chrono.lapTimeMs() << " ms" << std::endl;

    using RTree = bgi::rtree<std::pair<BoxGeo,size_t>, bgi::rstar<16,4>>;
    RTree rtree(regions | ba::indexed(0) | ba::transformed([](const auto& e) {
        return std::make_pair(bg::return_envelope<BoxGeo>(e.value().first), e.index());
    }));

    std::cout << "constructed R-tree in " << chrono.lapTimeMs() << " ms" << std::endl;

    PolygonGeo hull;
    if(area_provided) {
        MultipolygonGeo search_area = IO::parse_geojson_multipolygon(area_file);
        bg::convex_hull(search_area, hull);
    } else {
        bg::convert(rtree.bounds(), hull);
    }

    std::vector<H3Index> hex_indices = polyfill(hull, hex_resolution);
    std::vector<std::tuple<H3Index, double, double>> hex_datas;
    hex_datas.resize(hex_indices.size());

    std::transform(std::execution::par_unseq, hex_indices.cbegin(), hex_indices.cend(), hex_datas.begin(), [&rtree, &regions](const H3Index i){ 
        RingGeo r = indexToRing(i);
        PointGeo center = indexToCenter(i);
        srs::projection<> proj = srs::proj4("+proj=eqc +ellps=GRS80 +lon_0="+ std::to_string(center.get<0>()) +" +lat_0="+ std::to_string(center.get<1>()));
        Ring2D r2d;
        proj.forward(r, r2d);

        std::vector<std::pair<Multipolygon2D, RegionInfos>> intersected_regions2d;
        for(const auto & result : rtree | bgi::adaptors::queried(bgi::intersects(bg::return_envelope<BoxGeo>(r)) && bgi::satisfies([&regions, &r](const auto & v) {
                return bg::intersects(r, regions[v.second].first);}))) {
            intersected_regions2d.emplace_back(std::piecewise_construct, 
                    std::forward_as_tuple(), std::forward_as_tuple(regions[result.second].second));
            proj.forward(regions[result.second].first, intersected_regions2d.back().first);
        }

        boost::sort(intersected_regions2d, [](std::pair<Multipolygon2D, RegionInfos> & e1, std::pair<Multipolygon2D, RegionInfos> & e2){
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
        }
        sum_quality += DEFAULT_QUALITY_COEF * bg::area(mp2d);
        sum_prob += DEFAULT_PROB_COEF * bg::area(mp2d);

        return std::make_tuple(i, sum_quality/area, sum_prob/area);
    });

    std::cout << "Compelte polyfill in " << chrono.lapTimeMs() << " ms" << std::endl;

    if(generate_svg)
        IO::print_hexagons_svg(hex_datas, output_file.replace_extension(".svg"));

    std::cout << "Generate svg in " << chrono.lapTimeMs() << " ms" << std::endl;
    
    return EXIT_SUCCESS;
}