#ifndef WAY_HPP
#define WAY_HPP

#include <list>
#include <utility>
#include <vector>

#include "bg_types.hpp"

class Way {
public:
    const LinestringGeo linestring;
    const std::vector<std::pair<std::string,std::string>> properties;

    template <typename Linestring, typename Properties>
    Way(Linestring&& linestring, Properties&& properties)
        : linestring(std::forward<Linestring>(linestring))
        , properties(std::forward<Properties>(properties)) {}

    bool hasProperty(const std::string & tag) const noexcept {
        return std::any_of(properties.cbegin(), properties.cend(), [&tag](auto & p){ return p.first == tag; });
    }
    const std::string & getProperty(const std::string & tag) const noexcept {
        return std::find_if(properties.cbegin(), properties.cend(), [&tag](auto & p){ return p.first == tag; })->second;
    }
};

#endif // WAY_HPP
