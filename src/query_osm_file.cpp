#include "query_osm_file.hpp"
#include <osmium/util/progress_bar.hpp>

#include "osmium_utils/filteringMultipolygonManager.hpp"

using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

namespace bg = boost::geometry;
namespace ba = boost::adaptors;

void do_query(const osmium::io::File & osm_file, BGRegionsDumpHandler & bg_handler) {
    osmium::area::Assembler::config_type assembler_config;
    osmium::area::FilteringMultipolygonManager<osmium::area::Assembler> mp_manager{assembler_config, bg_handler.getAreaEnglobingFilter(), bg_handler.getSearchBox()};
    osmium::relations::read_relations(osm_file, mp_manager);

    osmium::io::Reader reader{osm_file};

    index_type index;
    location_handler_type location_handler{index};
    location_handler.ignore_errors();

    osmium::ProgressBar progress{reader.file_size(), osmium::isatty(2)};
    osmium::apply(reader, location_handler, bg_handler, mp_manager.handler([&bg_handler, &progress, &reader](osmium::memory::Buffer&& buffer) {
        osmium::apply(buffer, bg_handler);
        progress.update(reader.offset());
    }));
    progress.remove();
    progress.update(reader.offset());
    progress.done();

    reader.close();
}

MultipolygonGeo query_osm_search_area(const std::filesystem::path & input_file,
        const std::filesystem::path & search_area_pattern_file) {
    MultipolygonGeo search_area;

    osmium::io::File osm_file(input_file);
    std::ifstream search_area_pattern_stream(search_area_pattern_file);
    nlohmann::json search_area_pattern;
    search_area_pattern_stream >> search_area_pattern;

    BGRegionsDumpHandler bg_search_area_handler(
        std::vector<std::pair<std::vector<std::pair<std::string,osmium::StringMatcher>>,NodeRegionBuilder>>(),
        std::vector<std::pair<std::vector<std::pair<std::string,osmium::StringMatcher>>,WayRegionBuilder>>(),
        std::vector<std::pair<std::vector<std::pair<std::string,osmium::StringMatcher>>,AreaRegionBuilder>>(1, IO::parse_area_pattern(search_area_pattern))
    );
    do_query(osm_file, bg_search_area_handler);

    if(bg_search_area_handler.getRegions().size() == 0)
        throw std::runtime_error("Search Area not found");
    if(bg_search_area_handler.getRegions().size() > 1)
        throw std::runtime_error("Too much Search Area founded");

    PolygonGeo hull;
    bg::convex_hull(bg_search_area_handler.getRegions().front().multipolygon, hull);
    bg::convert(hull, search_area);
    
    return search_area;
}


std::vector<Region> query_osm_regions(const std::filesystem::path & input_file,
        const std::filesystem::path & patterns_file, 
        const MultipolygonGeo & search_area) {
    osmium::io::File osm_file(input_file);

    std::ifstream patterns_stream(patterns_file);
    nlohmann::json patterns;
    patterns_stream >> patterns;
    
    BGRegionsDumpHandler bg_handler(
        search_area,
        IO::parse_node_patterns(patterns["nodePatterns"]),
        IO::parse_way_patterns(patterns["wayPatterns"]),
        IO::parse_area_patterns(patterns["areaPatterns"])
    );
    do_query(osm_file, bg_handler);

    return bg_handler.getRegions();
}