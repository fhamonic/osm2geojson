#include <filesystem>  // filesystem::path
#include <fstream>     // ofstream
#include <iostream>    // std::cout, std::cerr

#include "bg_types.hpp"
#include "io/geojson_parser.hpp"
#include "io/print_geojson.hpp"
#include "io/print_svg_result.hpp"
#include "query_osm_file.hpp"

#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
namespace logging = boost::log;

void init_logging(bool no_warnings) {
    logging::core::get()->set_filter(
        no_warnings
            ? (logging::trivial::severity >= logging::trivial::error)
            : (logging::trivial::severity >= logging::trivial::warning));
}

static bool process_command_line(int argc, char * argv[],
                                 std::filesystem::path & input_file,
                                 std::filesystem::path & patterns_file,
                                 std::filesystem::path & output_file,
                                 std::filesystem::path & area_file,
                                 bool & provided_area_file, bool & generate_svg,
                                 bool & no_warnings) {
    try {
        bpo::options_description desc("Allowed options");
        desc.add_options()("help,h", "produce help message")(
            "input,i",
            bpo::value<std::filesystem::path>(&input_file)->required(),
            "set input PBF file")(
            "patterns,p",
            bpo::value<std::filesystem::path>(&patterns_file)->required(),
            "set regions patterns description json file")(
            "output,o",
            bpo::value<std::filesystem::path>(&output_file)->required(),
            "set output geojson file")(
            "search-area,a", bpo::value<std::filesystem::path>(&area_file),
            "set search area geojson file")(
            "svg", "generate the svg file of the result regions")(
            "no-warnings", "silence warning prints");
        bpo::positional_options_description p;
        p.add("input", 1).add("patterns", 1).add("output", 1);
        bpo::variables_map vm;
        bpo::store(bpo::command_line_parser(argc, argv)
                       .options(desc)
                       .positional(p)
                       .run(),
                   vm);
        if(vm.count("help")) {
            std::cout << desc << "\n";
            return false;
        }
        bpo::notify(vm);
        provided_area_file = (vm.count("search-area") > 0);
        generate_svg = (vm.count("svg") > 0);
        no_warnings = (vm.count("no-warnings") > 0);
    } catch(std::exception & e) {
        std::cerr << "Error: " << e.what() << "\n";
        return false;
    }
    return true;
}

#include "chrono.hpp"

int main(int argc, char * argv[]) {
    std::filesystem::path input_file;
    std::filesystem::path patterns_file;
    std::filesystem::path output_file;
    std::filesystem::path area_file;
    bool provided_area_file;
    bool generate_svg;
    bool no_warnings;

    bool valid_command = process_command_line(
        argc, argv, input_file, patterns_file, output_file, area_file,
        provided_area_file, generate_svg, no_warnings);
    if(!valid_command) return EXIT_FAILURE;
    init_logging(no_warnings);

    MultipolygonGeo search_area;
    if(provided_area_file) {
        simdjson::ondemand::parser parser;
        auto json = simdjson::padded_string::load(area_file);
        auto doc = parser.iterate(json);

        if(doc.find_field("type") != "FeatureCollection")
            throw std::runtime_error(area_file.filename().string() +
                                     " is not of type FeatureCollection");

        for(auto region : doc.find_field("features").get_array()) {
            auto geometry = region.find_field("geometry");
            if(geometry.find_field("type") != "MultiPolygon")
                throw std::runtime_error(
                    "region geometry with type != MultiPolygon");
            search_area =
                IO::detail::parse_geojson_multipolygon<MultipolygonGeo>(
                    geometry.find_field("coordinates").get_array());
            break;
        }
    }

    Chrono chrono;

    BGDumpHandler bg_handler =
        query_osm(input_file, patterns_file, search_area);
    std::cout << "Query result in " << chrono.lapTimeMs() << " ms" << std::endl;

    IO::print_geojson(bg_handler.getNodes(), bg_handler.getWays(),
                      bg_handler.getAreas(), output_file);
    std::cout << "Printed geojson in " << chrono.lapTimeMs() << " ms"
              << std::endl;

    if(generate_svg) {
        IO::print_svg_result(bg_handler.getNodes(), bg_handler.getWays(),
                             bg_handler.getAreas(),
                             output_file.replace_extension(".svg"));
        std::cout << "Printed svg in " << chrono.lapTimeMs() << " ms"
                  << std::endl;
    }
}