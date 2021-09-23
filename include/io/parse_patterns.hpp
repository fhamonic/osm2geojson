#ifndef PARSE_PATTERNS_HPP
#define PARSE_PATTERNS_HPP

#include <utility>
#include <vector>

#include <boost/range/algorithm.hpp>
#include <nlohmann/json.hpp>
#include <osmium/tags/tags_filter.hpp>
#include <osmium/util/string_matcher.hpp>

#include "builders.hpp"

namespace IO {
osmium::TagsFilter rules_to_tagsfilter(
    const std::vector<
        std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
                  AreaBuilder>> & rules);
std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
          NodeBuilder>
parse_node_pattern(const nlohmann::json & pattern);
std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
          WayBuilder>
parse_way_pattern(const nlohmann::json & pattern);
std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
          AreaBuilder>
parse_area_pattern(const nlohmann::json & pattern);

std::vector<
    std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
              NodeBuilder>>
parse_node_patterns(const nlohmann::json & patterns);
std::vector<
    std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
              WayBuilder>>
parse_way_patterns(const nlohmann::json & patterns);
std::vector<
    std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
              AreaBuilder>>
parse_area_patterns(const nlohmann::json & patterns);
}  // namespace IO

#endif  // PARSE_PATTERNS_HPP