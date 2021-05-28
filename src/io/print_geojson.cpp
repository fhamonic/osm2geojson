
#include "io/print_geojson.hpp"

namespace bg = boost::geometry;
namespace ba = boost::adaptors;

namespace IO {
    void print_geojson(const std::vector<Region> & regions, const std::filesystem::path & json_file) {
        std::ofstream json(json_file);
        json << std::setprecision(std::numeric_limits<double>::max_digits10);

        json << "{\"type\":\"FeatureCollection\",\"features\":[";
        for(const auto & e : regions | ba::indexed(0)) {
            json << "{\"type\":\"Feature\",\"geometry\":{\"type\":\"MultiPolygon\",\"coordinates\":" 
                << bg::dsv(e.value().multipolygon, ",", "[", "]", ",", "[", "]", ",")
                << "},\"properties\":{" << boost::algorithm::join(e.value().properties | ba::transformed([](const auto & p){ return "\""+p.first+"\":\""+p.second+"\""; }), ",")
                << "}}" << (e.index()+1 != static_cast<std::ptrdiff_t>(regions.size()) ? "," : "");
        }
        json << "]}";
    }
}
