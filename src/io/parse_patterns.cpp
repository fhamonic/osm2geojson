#include "io/parse_patterns.hpp"

namespace IO {
    static std::vector<std::pair<std::string,std::string>> parse_tags_rule(const nlohmann::json & rule) {
        std::vector<std::pair<std::string,std::string>> tags;
        if(!rule.contains("matchPropertiesValues"))
            throw std::runtime_error("pattern has not matchPropertiesValues:\n" + rule.dump());
        for(auto & [tag, value] : rule["matchPropertiesValues"].items())
            tags.emplace_back(tag, value.get<const std::string>());
        return tags;
    }

    std::pair<std::vector<std::pair<std::string,std::string>>, NodeRegionBuilder> parse_node_pattern(const nlohmann::json & pattern) {
        if(!pattern.contains("inflatedWidth"))
            throw std::runtime_error("node pattern has not inflatedWidth:\n" + pattern.dump());
        const float inflateWidth = pattern.at("inflatedWidth").get<float>();

        std::vector<std::pair<std::string, std::string>> exported_properties;
        if(pattern.contains("exportProperties"))
            for(auto & [tag, value] : pattern.at("exportProperties").items())
                exported_properties.emplace_back(tag, value.dump());

        std::vector<std::string> forwarded_properties;
        if(pattern.contains("forwardProperties"))
            for(auto & tag : pattern.at("forwardProperties"))
                forwarded_properties.emplace_back(tag.get<const std::string>());

        return std::make_pair(parse_tags_rule(pattern), NodeRegionBuilder(inflateWidth, std::move(exported_properties), std::move(forwarded_properties)));
    }
    std::vector<std::pair<std::vector<std::pair<std::string,std::string>>,NodeRegionBuilder>> parse_node_patterns(const nlohmann::json & patterns) {
        std::vector<std::pair<std::vector<std::pair<std::string,std::string>>,NodeRegionBuilder>> rules;
        rules.reserve(patterns.size());
        boost::transform(patterns, std::back_inserter(rules), &parse_node_pattern);
        return rules;
    }

    std::pair<std::vector<std::pair<std::string,std::string>>,WayRegionBuilder> parse_way_pattern(const nlohmann::json & pattern) {
        if(!pattern.contains("inflatedWidth"))
            throw std::runtime_error("way pattern has not inflatedWidth:\n" + pattern.dump());
        const float inflateWidth = pattern.at("inflatedWidth").get<float>();

        std::vector<std::pair<std::string, std::string>> exported_properties;
        if(pattern.contains("exportProperties"))
            for(auto & [tag, value] : pattern.at("exportProperties").items())
                exported_properties.emplace_back(tag, value.dump());

        std::vector<std::string> forwarded_properties;
        if(pattern.contains("forwardProperties"))
            for(auto & tag : pattern.at("forwardProperties"))
                forwarded_properties.emplace_back(tag.get<const std::string>());

        return std::make_pair(parse_tags_rule(pattern), WayRegionBuilder(inflateWidth, std::move(exported_properties), std::move(forwarded_properties)));
    }
    std::vector<std::pair<std::vector<std::pair<std::string,std::string>>,WayRegionBuilder>> parse_way_patterns(const nlohmann::json & patterns) {
        std::vector<std::pair<std::vector<std::pair<std::string,std::string>>,WayRegionBuilder>> rules;
        rules.reserve(patterns.size());
        boost::transform(patterns, std::back_inserter(rules), &parse_way_pattern);
        return rules;
    }

    std::pair<std::vector<std::pair<std::string,std::string>>, AreaRegionBuilder> parse_area_pattern(const nlohmann::json & pattern) {
        std::vector<std::pair<std::string, std::string>> exported_properties;
        if(pattern.contains("exportProperties"))
            for(auto & [tag, value] : pattern.at("exportProperties").items())
                exported_properties.emplace_back(tag, value.dump());

        std::vector<std::string> forwarded_properties;
        if(pattern.contains("forwardProperties"))
            for(auto & tag : pattern.at("forwardProperties"))
                forwarded_properties.emplace_back(tag.get<const std::string>());

        return std::make_pair(parse_tags_rule(pattern), AreaRegionBuilder(std::move(exported_properties), std::move(forwarded_properties)));
    }
    std::vector<std::pair<std::vector<std::pair<std::string,std::string>>, AreaRegionBuilder>> parse_area_patterns(const nlohmann::json & patterns) {
        std::vector<std::pair<std::vector<std::pair<std::string,std::string>>, AreaRegionBuilder>> rules;
        rules.reserve(patterns.size());
        boost::transform(patterns, std::back_inserter(rules), &parse_area_pattern);
        return rules;
    }
}
