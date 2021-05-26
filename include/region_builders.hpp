#ifndef REGION_BUILDERS_HPP
#define REGION_BUILDERS_HPP

#include <utility>
#include <vector>

#include "bg_types.hpp"
#include "bg_utils.hpp"
#include "region.hpp"

inline std::vector<std::pair<std::string,std::string>> forward_properties(
            const std::vector<std::pair<std::string,std::string>> & tags, const std::vector<std::string> & tags_to_forward) {
    std::vector<std::pair<std::string,std::string>> forwarded_properties;
    auto tags_first = tags.cbegin(); 
    auto tags_last = tags.cend(); 
    auto tags_to_forward_first = tags_to_forward.cbegin(); 
    auto tags_to_forward_last = tags_to_forward.cbegin();
    while(tags_first != tags_last && tags_to_forward_first != tags_to_forward_last) {
        int r_cmp = tags_first->first.compare(*tags_to_forward_first);
        if(r_cmp < 0) { ++tags_first; continue; }
        if(r_cmp > 0) { ++tags_to_forward_first; continue; }
        forwarded_properties.emplace_back(tags_first);
        ++tags_first; ++tags_to_forward_first;
    }
    return forwarded_properties;
}

class NodeRegionBuilder {
private:
    float inflateWidth;
    std::vector<std::string> tags_to_forward;

public:
    template <typename FProperties>
    NodeRegionBuilder(float inflateWidth, FProperties&& tags_to_forward)
        : inflateWidth(inflateWidth)
        , tags_to_forward(std::forward<FProperties>(tags_to_forward)) {}

    template <typename Point>
    Region build(const std::vector<std::pair<std::string,std::string>> & tags, Point&& p) const noexcept {        
        return Region(buffer_PointGeo(std::forward<Point>(p), inflateWidth), forward_properties(tags, tags_to_forward));
    }
};


class WayRegionBuilder {
private:
    float inflateWidth;
    std::vector<std::string> tags_to_forward;

public:
    template <typename FProperties>
    WayRegionBuilder(float inflateWidth, FProperties&& tags_to_forward)
        : inflateWidth(inflateWidth)
        , tags_to_forward(std::forward<FProperties>(tags_to_forward)) {}

    template <typename Linestring>
    Region build(const std::vector<std::pair<std::string,std::string>> & tags, Linestring&& l) const noexcept {        
        return Region(buffer_LinestringGeo(std::forward<Linestring>(l), inflateWidth), forward_properties(tags, tags_to_forward));
    }
};


class AreaRegionBuilder {
private:
    std::vector<std::string> tags_to_forward;

public:
    template <typename FProperties>
    AreaRegionBuilder(FProperties&& tags_to_forward)
        : tags_to_forward(std::forward<FProperties>(tags_to_forward)) {}

    template <typename Multipolygon>
    Region build(const std::vector<std::pair<std::string,std::string>> & tags, Multipolygon&& mp) const noexcept {        
        return Region(std::forward<Multipolygon>(mp), forward_properties(tags, tags_to_forward));
    }
};

#endif // REGION_BUILDERS_HPP