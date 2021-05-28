#ifndef REGION_BUILDERS_HPP
#define REGION_BUILDERS_HPP

#include <utility>
#include <vector>

#include "bg_types.hpp"
#include "bg_utils.hpp"
#include "region.hpp"

template <typename Tags>
std::vector<std::pair<std::string,std::string>> forward_properties(
            Tags&& tags, const std::vector<std::string> & tags_to_forward) {
    std::vector<std::pair<std::string,std::string>> forwarded_properties;
    auto tags_first = tags.cbegin();
    auto tags_last = tags.cend();
    auto tags_to_forward_first = tags_to_forward.cbegin();
    auto tags_to_forward_last = tags_to_forward.cend();
    while(tags_first != tags_last && tags_to_forward_first != tags_to_forward_last) {
        int r_cmp = tags_first->first.compare(*tags_to_forward_first);
        if(r_cmp < 0) { ++tags_first; continue; }
        if(r_cmp > 0) { ++tags_to_forward_first; continue; }
        forwarded_properties.emplace_back(*tags_first);
        ++tags_first; ++tags_to_forward_first;
    }
    return forwarded_properties;
}

class NodeRegionBuilder {
private:
    float inflateWidth;
    std::vector<std::pair<std::string,std::string>> properties;
    std::vector<std::string> tags_to_forward;

public:
    template <typename EProperties, typename FProperties>
    NodeRegionBuilder(float inflateWidth, EProperties&& properties, FProperties&& tags_to_forward)
            : inflateWidth(inflateWidth)
            , properties(std::forward<EProperties>(properties))
            , tags_to_forward(std::forward<FProperties>(tags_to_forward)) {
        boost::sort(properties);
        boost::sort(tags_to_forward);
    }

    template <typename Tags, typename Point>
    Region build(Tags&& tags, Point&& p) const noexcept {
        std::vector<std::pair<std::string,std::string>> build_properties = forward_properties(std::forward<Tags>(tags), tags_to_forward);
        boost::copy(properties, std::back_inserter(build_properties));
        return Region(buffer_PointGeo(std::forward<Point>(p), inflateWidth), std::move(build_properties));
    }
};


class WayRegionBuilder {
private:
    float inflateWidth;
    std::vector<std::pair<std::string,std::string>> properties;
    std::vector<std::string> tags_to_forward;

public:
    template <typename EProperties, typename FProperties>
    WayRegionBuilder(float inflateWidth, EProperties&& properties, FProperties&& tags_to_forward)
            : inflateWidth(inflateWidth)
            , properties(std::forward<EProperties>(properties))
            , tags_to_forward(std::forward<FProperties>(tags_to_forward)) {
        boost::sort(properties);
        boost::sort(tags_to_forward);
    }

    template <typename Tags, typename Linestring>
    Region build(Tags&& tags, Linestring&& l) const noexcept {
        std::vector<std::pair<std::string,std::string>> build_properties = forward_properties(std::forward<Tags>(tags), tags_to_forward);
        boost::copy(properties, std::back_inserter(build_properties));
        return Region(buffer_LinestringGeo(std::forward<Linestring>(l), inflateWidth), std::move(build_properties));
    }
};


class AreaRegionBuilder {
private:
    std::vector<std::pair<std::string,std::string>> properties;
    std::vector<std::string> tags_to_forward;

public:
    template <typename EProperties, typename FProperties>
    AreaRegionBuilder(EProperties&& properties, FProperties&& tags_to_forward)
            : properties(std::forward<EProperties>(properties))
            , tags_to_forward(std::forward<FProperties>(tags_to_forward)) {
        boost::sort(properties);
        boost::sort(tags_to_forward);
    }

    template <typename Tags, typename Multipolygon>
    Region build(Tags&& tags, Multipolygon&& mp) const noexcept {
        std::vector<std::pair<std::string,std::string>> build_properties = forward_properties(std::forward<Tags>(tags), tags_to_forward);
        boost::copy(properties, std::back_inserter(build_properties));
        return Region(std::forward<Multipolygon>(mp), std::move(build_properties));
    }
};

#endif // REGION_BUILDERS_HPP