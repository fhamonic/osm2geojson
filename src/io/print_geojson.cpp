
#include "io/print_geojson.hpp"

namespace bg = boost::geometry;
namespace ba = boost::adaptors;

namespace IO {
    void print_geojson(const std::vector<Region> & regions, const std::filesystem::path & json_file) {
        std::ofstream json(json_file);
        json << std::setprecision(std::numeric_limits<double>::max_digits10);

        json << "{\"type\": \"FeatureCollection\"," << std::endl
            << "\t\"features\": [" << std::endl;

        for(const auto & e : regions | ba::indexed(0)) {
            json << "\t\t{ \"type\": \"Feature\"," << std::endl
                << "\t\t\t\"geometry\": { \"type\": \"MultiPolygon\"," << std::endl
                << "\t\t\t\t\"coordinates\":" << bg::dsv(e.value().multipolygon, ", ", "[", "]", ", ", "[ ", " ]", ", ") << std::endl
                << "\t\t\t}," << std::endl
                << "\t\t\t\"properties\": {" << std::endl;
            json << "\t\t\t\t" << boost::algorithm::join(e.value().properties | ba::transformed([](const auto & p){ return "\""+p.first+"\":\""+p.second+"\""; }), ", ") << std::endl;
            json << "\t\t\t}" << std::endl
                << "\t\t}" << (e.index()+1 != static_cast<std::ptrdiff_t>(regions.size()) ? "," : "") << std::endl;
        }

        json << "\t]" << std::endl 
            << "}";
    }
}
