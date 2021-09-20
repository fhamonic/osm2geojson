#ifndef NODE_HPP
#define NODE_HPP

#include <list>
#include <utility>
#include <vector>

#include "bg_types.hpp"

class Node {
public:
    const PointGeo point;
    const std::vector<std::pair<std::string,std::string>> properties;

    template <typename Point, typename Properties>
    Node(Point&& point, Properties&& properties)
        : point(std::forward<Point>(point))
        , properties(std::forward<Properties>(properties)) {}

    bool hasProperty(const std::string & tag) const noexcept {
        return std::any_of(properties.cbegin(), properties.cend(), [&tag](auto & p){ return p.first == tag; });
    }
    const std::string & getProperty(const std::string & tag) const noexcept {
        return std::find_if(properties.cbegin(), properties.cend(), [&tag](auto & p){ return p.first == tag; })->second;
    }
};

#endif // NODE_HPP
