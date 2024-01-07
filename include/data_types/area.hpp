#ifndef AREA_HPP
#define AREA_HPP

#include <list>
#include <utility>
#include <vector>

#include "bg_types.hpp"

class Area {
public:
    const MultipolygonGeo multipolygon;
    const std::vector<std::pair<std::string, std::string>> properties;

    template <typename Multipolygon, typename Properties>
    Area(Multipolygon && multipolygon, Properties && properties)
        : multipolygon(std::forward<Multipolygon>(multipolygon))
        , properties(std::forward<Properties>(properties)) {}

    bool hasProperty(const std::string & tag) const noexcept {
        return std::any_of(properties.cbegin(), properties.cend(),
                           [&tag](auto & p) { return p.first == tag; });
    }
    const std::string & getProperty(const std::string & tag) const noexcept {
        return std::find_if(properties.cbegin(), properties.cend(),
                            [&tag](auto & p) { return p.first == tag; })
            ->second;
    }
};

#endif  // AREA_HPP
