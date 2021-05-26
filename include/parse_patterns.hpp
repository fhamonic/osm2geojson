#ifndef PARSE_PATTERNS_HPP
#define PARSE_PATTERNS_HPP

#include <list>
#include <utility>
#include <vector>

#include <osmium/tags/tags_filter.hpp>

#include <nlohmann/json.hpp>

#include "region_builders.hpp"

osmium::TagsFilter rules_to_tagsfilter(const std::vector<std::pair<std::vector<std::pair<std::string,std::string>>,AreaRegionBuilder>> & rules);
std::pair<std::vector<std::pair<std::string,std::string>>, NodeRegionBuilder> parse_node_pattern(const nlohmann::json & pattern);
std::pair<std::vector<std::pair<std::string,std::string>>, WayRegionBuilder> parse_way_pattern(const nlohmann::json & pattern);
std::pair<std::vector<std::pair<std::string,std::string>>, AreaRegionBuilder> parse_area_pattern(const nlohmann::json & pattern);

std::vector<std::pair<std::vector<std::pair<std::string,std::string>>,NodeRegionBuilder>> parse_node_patterns(const nlohmann::json & patterns);
std::vector<std::pair<std::vector<std::pair<std::string,std::string>>,WayRegionBuilder>> parse_way_patterns(const nlohmann::json & patterns);
std::vector<std::pair<std::vector<std::pair<std::string,std::string>>, AreaRegionBuilder>> parse_area_patterns(const nlohmann::json & patterns);

#endif // PARSE_PATTERNS_HPP