#include "query_osm_file.hpp"

using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

namespace bg = boost::geometry;


template <typename NInfo, typename WInfo, typename AInfo>
void do_query(const osmium::io::File & osm_file, const osmium::TagsFilter & area_englobing_filter, BGRegionsDumpHandler<NInfo,WInfo,AInfo> & bg_handler) {
    osmium::area::Assembler::config_type assembler_config;
    osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager{assembler_config, area_englobing_filter};
    osmium::relations::read_relations(osm_file, mp_manager);

    osmium::io::Reader reader{osm_file};

    index_type index;
    location_handler_type location_handler{index};
    location_handler.ignore_errors();

    osmium::apply(reader, location_handler, bg_handler, mp_manager.handler([&bg_handler](osmium::memory::Buffer&& buffer) {
        osmium::apply(buffer, bg_handler);
    }));
    reader.close();
}

std::vector<Region> query_osm_file(const std::filesystem::path & input_file,
        const std::filesystem::path & patterns_file, 
        const std::filesystem::path & search_area_pattern_file) {
    osmium::io::File osm_file(input_file);

    std::ifstream patterns_stream(patterns_file);
    nlohmann::json patterns;
    patterns_stream >> patterns;

    MultipolygonGeo search_area;

    if(!search_area_pattern_file.empty()) {
        std::ifstream search_area_pattern_stream(search_area_pattern_file);
        nlohmann::json search_area_pattern;
        search_area_pattern_stream >> search_area_pattern;
        
        const osmium::TagsFilter search_area_filter = jsonpattern_to_tagsfilter(search_area_pattern);
        const osmium::TagsFilter area_englobing_filter = jsonpatterns_to_englobing_tagsfilter(patterns["areaPatterns"]);

        BGRegionsDumpHandler bg_search_area_handler(
            std::vector<std::pair<osmium::MyTagsFilter, int>>(),
            std::vector<std::pair<osmium::MyTagsFilter, int>>(),
            std::vector<std::pair<osmium::MyTagsFilter, int>>(1, std::make_pair(jsonpattern_to_mytagsfilter(search_area_pattern), 0))
        );
        do_query(osm_file, search_area_filter, bg_search_area_handler);
        
        if(bg_search_area_handler.getAreas().size() != 1)
            throw "Search Area not found";

        PolygonGeo hull;
        bg::convex_hull(bg_search_area_handler.getAreas().front().first, hull);
        bg::convert(hull, search_area);
    }

    const osmium::TagsFilter area_englobing_filter = jsonpatterns_to_englobing_tagsfilter(patterns["areaPatterns"]);

    BGRegionsDumpHandler<RegionInfo,RegionInfo,RegionInfo> bg_handler(
        search_area,
        parse_patterns(patterns["nodePatterns"]),
        parse_patterns(patterns["wayPatterns"]),
        parse_patterns(patterns["areaPatterns"])
    );
    do_query(osm_file, area_englobing_filter, bg_handler);

    return bg_handler.getRegion();
}