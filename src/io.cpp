
#include "io.hpp"

namespace bg = boost::geometry;
namespace ba = boost::adaptors;

void plot_svg(const std::vector<Region> & raw_regions, const std::filesystem::path & svg_file) {
    std::ofstream svg(svg_file);
    bg::svg_mapper<PointGeo> mapper(svg, 1500, 1500);

    if(!std::all_of(raw_regions.cbegin(), raw_regions.cend(), [](const auto & p){ return p.hasProperty("qualityCoef") && p.hasProperty("probConnectionPerMeter"); }))
        throw std::runtime_error("generate-svg needs \"qualityCoef\" and \"probConnectionPerMeter\" exported properties for every regions");

    std::vector<std::pair<MultipolygonGeo,std::pair<double, double>>> regions(raw_regions.size());
    try {
        std::transform(raw_regions.cbegin(), raw_regions.cend(), regions.begin(), [](const Region & r){ 
            return std::make_pair(r.multipolygon, std::make_pair(std::atof(r.getProperty("qualityCoef").c_str()), 
                        std::atof(r.getProperty("probConnectionPerMeter").c_str()))); });
    } catch(std::invalid_argument & e) {
        throw std::runtime_error("generate-svg needs \"qualityCoef\" and \"probConnectionPerMeter\" exported properties to be numbers");
    }

    float min_quality = *boost::min_element(regions | ba::transformed([](const auto & p){ return p.second.first; }));
    float max_quality = *boost::max_element(regions | ba::transformed([](auto & p){ return p.second.first; }));
    float min_resistance = *boost::min_element(regions | ba::transformed([](auto & p){ return p.second.second; }));
    float max_resistance = *boost::max_element(regions | ba::transformed([](auto & p){ return p.second.second; }));

    for(auto & e : regions)
        mapper.add(e.first);


    for(auto & e : regions) {
        const MultipolygonGeo & mp = e.first;
        const double qualityCoef = e.second.first;
        const double probConnectionPerMeter = e.second.second;

        int green = 255 * ((max_quality-min_quality)!=0 ? (qualityCoef-min_quality)/(max_quality-min_quality) : 1);
        int red =  255 - 255 * ((max_resistance-min_resistance)!=0 ? (probConnectionPerMeter-min_resistance)/(max_resistance-min_resistance) : 1);

        mapper.map(mp, "fill-opacity:0.5;fill:rgb(" + std::to_string(red) + "," + std::to_string(green) + ",0);stroke:rgb(0,0,0);stroke-width:0");
    }
}

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
        json << "\t\t\t\t" << boost::algorithm::join(e.value().properties | ba::transformed([](const auto & p){ return "\""+p.first+"\":"+p.second; }), ", ") << std::endl;
        json << "\t\t\t}" << std::endl
            << "\t\t}" << (e.index()+1 != static_cast<std::ptrdiff_t>(regions.size()) ? "," : "") << std::endl;
    }

    json << "\t]" << std::endl 
        << "}";
}

#include <simdjson.h>

std::vector<Region> parse_geojson(const std::filesystem::path & json_file) {
    std::vector<Region> regions;
    simdjson::ondemand::parser parser;
    auto json = simdjson::padded_string::load(json_file);
    simdjson::ondemand::document doc = parser.iterate(json);

    if(doc.find_field("type") != "FeatureCollection")
        throw std::runtime_error("json_file is not FeatureCollection");

    for(auto region : doc.find_field("features")) {
        
        if(region.find_field("type") != "MultiPolygon")
            throw std::runtime_error("region with type != MultiPolygon");

        for(auto polygon : region["geometry"]) {
            auto begin_ring = polygon.begin();
            auto end_ring = polygon.end();
            if(begin_ring == end_ring)
                throw std::runtime_error("region with type != MultiPolygon");
        }

        for (auto [key, value] : region["properties"]) {
            cout << key << " = " << value << endl;
        }

    }


    return regions;
}
