
#include "io/print_geojson.hpp"

namespace bg = boost::geometry;
namespace ba = boost::adaptors;

namespace IO {
void print_nodes(std::ofstream & os, const std::vector<Node> & nodes) {
    for(const auto & e : nodes | ba::indexed(0)) {
        os << "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
              "\"coordinates\":"
           << bg::dsv(e.value().point, ",", "[", "]", ",", "[", "]", ",")
           << "},\"properties\":{"
           << boost::algorithm::join(
                  e.value().properties | ba::transformed([](const auto & p) {
                      return "\"" + p.first + "\":\"" + p.second + "\"";
                  }),
                  ",")
           << "}}"
           << (e.index() + 1 != static_cast<std::ptrdiff_t>(nodes.size()) ? ","
                                                                          : "");
    }
}

void print_ways(std::ofstream & os, const std::vector<Way> & ways) {
    for(const auto & e : ways | ba::indexed(0)) {
        os << "{\"type\":\"Feature\",\"geometry\":{\"type\":\"LineString\","
              "\"coordinates\":"
           << bg::dsv(e.value().linestring, ",", "[", "]", ",", "[", "]", ",")
           << "},\"properties\":{"
           << boost::algorithm::join(
                  e.value().properties | ba::transformed([](const auto & p) {
                      return "\"" + p.first + "\":\"" + p.second + "\"";
                  }),
                  ",")
           << "}}"
           << (e.index() + 1 != static_cast<std::ptrdiff_t>(ways.size()) ? ","
                                                                         : "");
    }
}

void print_areas(std::ofstream & os, const std::vector<Area> & areas) {
    for(const auto & e : areas | ba::indexed(0)) {
        os << "{\"type\":\"Feature\",\"geometry\":{\"type\":\"MultiPolygon\","
              "\"coordinates\":"
           << bg::dsv(e.value().multipolygon, ",", "[", "]", ",", "[", "]", ",")
           << "},\"properties\":{"
           << boost::algorithm::join(
                  e.value().properties | ba::transformed([](const auto & p) {
                      return "\"" + p.first + "\":\"" + p.second + "\"";
                  }),
                  ",")
           << "}}"
           << (e.index() + 1 != static_cast<std::ptrdiff_t>(areas.size()) ? ","
                                                                          : "");
    }
}

void print_geojson(const std::vector<Node> & nodes,
                   const std::vector<Way> & ways,
                   const std::vector<Area> & areas,
                   const std::filesystem::path & json_file) {
    std::ofstream json(json_file);
    json << std::setprecision(std::numeric_limits<double>::max_digits10);

    json << "{\"type\":\"FeatureCollection\",\"features\":[";
    print_nodes(json, nodes);
    if(nodes.size() > 0) json << ",";
    print_ways(json, ways);
    if(nodes.size() + ways.size() > 0) json << ",";
    print_areas(json, areas);
    json << "]}";
}
}  // namespace IO
