
#include "io/parse_geojson.hpp"

namespace bg = boost::geometry;
namespace ba = boost::adaptors;

namespace IO {
    template <typename T>
    PointGeo parse_geojson_point(T&& t) {
        PointGeo p;
        auto begin = t.begin();
        auto end = t.end();
        if(begin == end)
            throw std::runtime_error("point with no coordinates");
        p.x((*begin).get_double());
        if(++begin == end)
            throw std::runtime_error("point with only 1 coordinate");
        p.y((*begin).get_double());
        if(++begin != end)
            throw std::runtime_error("point with more than 2 coordinates");
        return p;
    }

    template <typename T>
    RingGeo parse_geojson_ring(T&& ring) {
        RingGeo r;
        for(auto point : ring)
            r.emplace_back(parse_geojson_point(point.get_array())); 
        return r;
    }

    template <typename T>
    PolygonGeo parse_geojson_polygon(T&& polygon) {
        PolygonGeo p;
        auto begin = polygon.begin();
        auto end = polygon.end();
        if(begin == end)
            throw std::runtime_error("region with empty polygon");
        p.outer() = parse_geojson_ring((*begin).get_array());
        for(++begin; begin!=end; ++begin)
            p.inners().emplace_back(parse_geojson_ring((*begin).get_array()));        
        return p;
    }

    template <typename T>
    MultipolygonGeo parse_geojson_multipolygon(T&& multipolygon) {
        MultipolygonGeo mp;
        for(auto polygon : multipolygon)
            mp.emplace_back(parse_geojson_polygon(polygon.get_array()));
        return mp;
    }

    template <typename T>
    std::vector<std::pair<std::string,std::string>> parse_geojson_properties(T&& properties) {
        std::vector<std::pair<std::string,std::string>> prop;
        for(auto p : properties)
            prop.emplace_back(p.unescaped_key().value(), p.value().get_string().value());
        return prop;
    }

    std::vector<Region> parse_geojson(const std::filesystem::path & json_file) {
        std::vector<Region> regions;
        simdjson::ondemand::parser parser;
        auto json = simdjson::padded_string::load(json_file);
        simdjson::ondemand::document doc = parser.iterate(json);
        simdjson::ondemand::object obj = doc.get_object();

        if(obj.find_field("type") != "FeatureCollection")
            throw std::runtime_error("json_file is not FeatureCollection");

        for(auto region : obj.find_field("features").get_array()) {
            auto geometry = region.find_field("geometry");

            if(geometry.find_field("type") != "MultiPolygon")
                throw std::runtime_error("region geometry with type != MultiPolygon");

            regions.emplace_back(parse_geojson_multipolygon(geometry["coordinates"].get_array()), parse_geojson_properties(region["properties"].get_object()));
        }
        return regions;
    }
}
