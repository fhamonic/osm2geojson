#ifndef REGION_HPP
#define REGION_HPP

#include <list>
#include <utility>
#include <vector>

#include "bg_types.hpp"

class Region {
public:
    const MultipolygonGeo multipolygon;
    const std::vector<std::pair<std::string,std::string>> properties;

    template <typename Multipolygon, typename Properties>
    Region(Multipolygon&& multipolygon, Properties&& properties)
        : multipolygon(multipolygon)
        , properties(std::forward<Properties>(properties)) {}

    bool hasProperty(const std::string & tag) const noexcept {
        return std::any_of(properties.cbegin(), properties.cend(), [&tag](auto & p){ return p.first == tag; });
    }
    const std::string & getProperty(const std::string & tag) const noexcept {
        return std::find_if(properties.cbegin(), properties.cend(), [&tag](auto & p){ return p.first == tag; })->second;
    }
};

#endif // REGION_HPP
