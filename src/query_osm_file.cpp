#include "query_osm_file.hpp"
#include <osmium/util/progress_bar.hpp>

#include "osmium_utils/filteringMultipolygonManager.hpp"

using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type,
                                               osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

namespace bg = boost::geometry;
namespace ba = boost::adaptors;

void do_query(const osmium::io::File & osm_file, BGDumpHandler & bg_handler) {
    osmium::area::Assembler::config_type assembler_config;
    osmium::area::FilteringMultipolygonManager<osmium::area::Assembler>
        mp_manager{assembler_config, bg_handler.getAreaEnglobingFilter(),
                   bg_handler.getSearchBox()};
    osmium::relations::read_relations(osm_file, mp_manager);

    osmium::io::Reader reader{osm_file};

    index_type index;
    location_handler_type location_handler{index};
    location_handler.ignore_errors();

    osmium::ProgressBar progress{reader.file_size(), osmium::isatty(2)};
    osmium::apply(reader, location_handler, bg_handler,
                  mp_manager.handler([&bg_handler, &progress, &reader](
                                         osmium::memory::Buffer && buffer) {
                      osmium::apply(buffer, bg_handler);
                      progress.update(reader.offset());
                  }));
    progress.remove();
    progress.update(reader.offset());
    progress.done();

    reader.close();
}

BGDumpHandler query_osm(const std::filesystem::path & input_file,
                        const std::filesystem::path & patterns_file,
                        const MultipolygonGeo & search_area) {
    osmium::io::File osm_file(input_file);

    std::ifstream patterns_stream(patterns_file);
    nlohmann::json patterns;
    patterns_stream >> patterns;

    BGDumpHandler bg_handler(search_area,
                             IO::parse_node_patterns(patterns["nodePatterns"]),
                             IO::parse_way_patterns(patterns["wayPatterns"]),
                             IO::parse_area_patterns(patterns["areaPatterns"]));
    do_query(osm_file, bg_handler);

    return bg_handler;
}