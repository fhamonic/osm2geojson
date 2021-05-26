
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

        int value = 127 * ((max_quality-min_quality)!=0 ? (qualityCoef-min_quality)/(max_quality-min_quality) : 1) + 
            128 - 127 * ((max_resistance-min_resistance)!=0 ? (probConnectionPerMeter-min_resistance)/(max_resistance-min_resistance) : 1);

        mapper.map(mp, "fill-opacity:0.5;fill:rgb(" + std::to_string(255 - value) + "," + std::to_string(value) + ",0);stroke:rgb(0,0,0);stroke-width:0");
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

std::vector<Region> parse_geojson(const std::filesystem::path & json_file) {
    std::vector<Region> regions;
    // TODO
    return regions;
}
