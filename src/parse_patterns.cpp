#include "parse_patterns.hpp"

osmium::TagsFilter tags_to_tagsfilter(const std::vector<std::pair<std::string,std::string>> & tags) {
    osmium::TagsFilter filter{false};
    for(auto & [tag, value] : tags)
        filter.add_rule(true, tag, value);
    return filter;
}

static std::vector<std::pair<std::string,std::string>> parse_tags_rule(const nlohmann::json & rule) {
    std::vector<std::pair<std::string,std::string>> tags;
    if(rule.contains("matchPropertiesValues"))
        for(auto & [tag, value] : rule["matchPropertiesValues"].items())
            tags.emplace_back(tag, value.get<const std::string>());
    return tags;
}

std::pair<std::vector<std::pair<std::string,std::string>>, NodeRegionBuilder> parse_node_pattern(const nlohmann::json & pattern) {
    if(!pattern.contains("inflatedWidth"))
        throw std::runtime_error("node pattern has not inflatedWidth:\n" + pattern.dump());
    const float inflateWidth = pattern.at("inflatedWidth").get<float>();

    if(!pattern.contains("exportProperties"))
        throw std::runtime_error("pattern has not exported properties:\n" + pattern.dump());
    std::vector<std::pair<std::string, std::string>> exported_properties;
    for(auto & [tag, value] : pattern.at("exportProperties").items())
        exported_properties.emplace_back(tag, value.dump());

    return std::make_pair(parse_tags_rule(pattern), NodeRegionBuilder(inflateWidth, exported_properties));
}

std::pair<std::vector<std::pair<std::string,std::string>>, WayRegionBuilder> parse_way_pattern(const nlohmann::json & pattern) {
    if(!pattern.contains("inflatedWidth"))
        throw std::runtime_error("way pattern has not inflatedWidth:\n" + pattern.dump());
    const float inflateWidth = pattern.at("inflatedWidth").get<float>();

    if(!pattern.contains("exportProperties"))
        throw std::runtime_error("pattern has not exported properties:\n" + pattern.dump());
    std::vector<std::pair<std::string, std::string>> exported_properties;
    for(auto & [tag, value] : pattern.at("exportProperties").items())
        exported_properties.emplace_back(tag, value.dump());

    return std::make_pair(parse_tags_rule(pattern), WayRegionBuilder(inflateWidth, exported_properties));
}

std::pair<std::vector<std::pair<std::string,std::string>>, AreaRegionBuilder> parse_area_pattern(const nlohmann::json & pattern) {
    if(!pattern.contains("exportProperties"))
        throw std::runtime_error("pattern has not exported properties:\n" + pattern.dump());
    std::vector<std::pair<std::string, std::string>> exported_properties;
    for(auto & [tag, value] : pattern.at("exportProperties").items())
        exported_properties.emplace_back(tag, value.dump());

    return std::make_pair(parse_tags_rule(pattern), AreaRegionBuilder(exported_properties));
}