#include <filesystem>  // filesystem::path
#include <fstream>  // ofstream
#include <iostream> // std::cout, std::cerr

#include "bg_types.hpp"
#include "query_osm_file.hpp"
#include "io/print_geojson.hpp"
#include "io/print_svg.hpp"

#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
namespace logging = boost::log;

void init_logging(bool no_warnings){
    logging::core::get()->set_filter(
        no_warnings
        ? (logging::trivial::severity >= logging::trivial::error)
        : (logging::trivial::severity >= logging::trivial::warning)
    );
}

static bool process_command_line(int argc, char* argv[], 
        std::filesystem::path & input_file, std::filesystem::path & patterns_file,
        std::filesystem::path & output_file, std::filesystem::path & area_file,
        bool & generate_svg, bool & no_warnings) {
    try {
        bpo::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("input,i", bpo::value<std::filesystem::path>(&input_file)->required(), "set input PBF file")
            ("patterns,p", bpo::value<std::filesystem::path>(&patterns_file)->required(), "set regions patterns description json file")
            ("output,o", bpo::value<std::filesystem::path>(&output_file)->required(), "set output geojson file")
            ("search-area,a", bpo::value<std::filesystem::path>(&area_file), "set search area pattern file")
            ("generate-svg", "generate the svg file of the result regions")
            ("no-warnings", "silence warning prints")
        ;
        bpo::positional_options_description p;
        p.add("input", 1).add("patterns", 1).add("output", 1);
        bpo::variables_map vm;
        bpo::store(bpo::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
        if(vm.count("help")) {
            std::cout << desc << "\n";
            return false;
        }
        bpo::notify(vm); 
        generate_svg = (vm.count("generate-svg") > 0);
        no_warnings = (vm.count("no-warnings") > 0);
    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::filesystem::path input_file;
    std::filesystem::path patterns_file;
    std::filesystem::path output_file;
    std::filesystem::path area_pattern_file;
    bool generate_svg;
    bool no_warnings;

    bool valid_command = process_command_line(argc, argv, input_file, patterns_file, output_file, area_pattern_file, generate_svg, no_warnings);
    if(!valid_command)
        return EXIT_FAILURE;
    init_logging(no_warnings);

    std::vector<Region> regions = query_osm_file(input_file, patterns_file, area_pattern_file);

    IO::print_geojson(regions, output_file);
    if(generate_svg)
        IO::print_svg(regions, output_file.replace_extension(".svg"));
}