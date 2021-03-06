#include "io/parse_patterns.hpp"

namespace IO {
static std::vector<std::pair<std::string, osmium::StringMatcher>>
parse_tags_rule(const nlohmann::json & rule) {
    std::vector<std::pair<std::string, osmium::StringMatcher>> tags;
    if(rule.contains("matchPropertiesValues"))
        for(auto & [tag, value] : rule["matchPropertiesValues"].items())
            tags.emplace_back(
                tag, osmium::StringMatcher{value.get<const std::string>()});
    if(rule.contains("requireProperties"))
        for(auto & tag : rule["requireProperties"])
            tags.emplace_back(tag, osmium::StringMatcher{true});
    if(tags.empty())
        throw std::runtime_error("pattern has no matching rules:\n" +
                                 rule.dump());
    return tags;
}

std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
          NodeBuilder>
parse_node_pattern(const nlohmann::json & pattern) {
    std::vector<std::pair<std::string, std::string>> exported_properties;
    if(pattern.contains("exportProperties"))
        for(auto & [tag, value] : pattern.at("exportProperties").items())
            exported_properties.emplace_back(tag, value.dump());

    std::vector<std::string> forwarded_properties;
    if(pattern.contains("forwardProperties"))
        for(auto & tag : pattern.at("forwardProperties"))
            forwarded_properties.emplace_back(tag.get<const std::string>());

    return std::make_pair(
        parse_tags_rule(pattern),
        NodeBuilder(std::move(exported_properties),
                    std::move(forwarded_properties)));
}
std::vector<std::pair<
    std::vector<std::pair<std::string, osmium::StringMatcher>>, NodeBuilder>>
parse_node_patterns(const nlohmann::json & patterns) {
    std::vector<
        std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
                  NodeBuilder>>
        rules;
    rules.reserve(patterns.size());
    boost::transform(patterns, std::back_inserter(rules), &parse_node_pattern);
    return rules;
}

std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
          WayBuilder>
parse_way_pattern(const nlohmann::json & pattern) {
    std::vector<std::pair<std::string, std::string>> exported_properties;
    if(pattern.contains("exportProperties"))
        for(auto & [tag, value] : pattern.at("exportProperties").items())
            exported_properties.emplace_back(tag, value.dump());

    std::vector<std::string> forwarded_properties;
    if(pattern.contains("forwardProperties"))
        for(auto & tag : pattern.at("forwardProperties"))
            forwarded_properties.emplace_back(tag.get<const std::string>());

    return std::make_pair(
        parse_tags_rule(pattern),
        WayBuilder(std::move(exported_properties),
                   std::move(forwarded_properties)));
}
std::vector<std::pair<
    std::vector<std::pair<std::string, osmium::StringMatcher>>, WayBuilder>>
parse_way_patterns(const nlohmann::json & patterns) {
    std::vector<std::pair<
        std::vector<std::pair<std::string, osmium::StringMatcher>>, WayBuilder>>
        rules;
    rules.reserve(patterns.size());
    boost::transform(patterns, std::back_inserter(rules), &parse_way_pattern);
    return rules;
}

std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
          AreaBuilder>
parse_area_pattern(const nlohmann::json & pattern) {
    std::vector<std::pair<std::string, std::string>> exported_properties;
    if(pattern.contains("exportProperties"))
        for(auto & [tag, value] : pattern.at("exportProperties").items())
            exported_properties.emplace_back(tag, value.dump());

    std::vector<std::string> forwarded_properties;
    if(pattern.contains("forwardProperties"))
        for(auto & tag : pattern.at("forwardProperties"))
            forwarded_properties.emplace_back(tag.get<const std::string>());

    return std::make_pair(parse_tags_rule(pattern),
                          AreaBuilder(std::move(exported_properties),
                                      std::move(forwarded_properties)));
}
std::vector<std::pair<
    std::vector<std::pair<std::string, osmium::StringMatcher>>, AreaBuilder>>
parse_area_patterns(const nlohmann::json & patterns) {
    std::vector<
        std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
                  AreaBuilder>>
        rules;
    rules.reserve(patterns.size());
    boost::transform(patterns, std::back_inserter(rules), &parse_area_pattern);
    return rules;
}
}  // namespace IO
