#ifndef BUILDERS_HPP
#define BUILDERS_HPP

#include <utility>
#include <vector>

#include "bg_types.hpp"
#include "bg_utils.hpp"

#include "data_types/area.hpp"
#include "data_types/node.hpp"
#include "data_types/way.hpp"

template <typename Tags>
std::vector<std::pair<std::string, std::string>> forward_properties(
    Tags && tags, const std::vector<std::string> & tags_to_forward) {
    std::vector<std::pair<std::string, std::string>> forwarded_properties;
    auto tags_first = tags.cbegin();
    auto tags_last = tags.cend();
    auto tags_to_forward_first = tags_to_forward.cbegin();
    auto tags_to_forward_last = tags_to_forward.cend();
    while(tags_first != tags_last &&
          tags_to_forward_first != tags_to_forward_last) {
        int r_cmp = tags_first->first.compare(*tags_to_forward_first);
        if(r_cmp < 0) {
            ++tags_first;
            continue;
        }
        if(r_cmp > 0) {
            ++tags_to_forward_first;
            continue;
        }
        forwarded_properties.emplace_back(*tags_first);
        ++tags_first;
        ++tags_to_forward_first;
    }
    return forwarded_properties;
}

class NodeBuilder {
private:
    std::vector<std::pair<std::string, std::string>> properties_to_export;
    std::vector<std::string> tags_to_forward;

public:
    template <typename EProperties, typename FProperties>
    NodeBuilder(EProperties && properties, FProperties && tags_to_forward)
        : properties_to_export(std::forward<EProperties>(properties))
        , tags_to_forward(std::forward<FProperties>(tags_to_forward)) {
        boost::sort(properties);
        boost::sort(tags_to_forward);
    }

    template <typename Tags, typename Point>
    Node build(Tags && tags, Point && p) const {
        std::vector<std::pair<std::string, std::string>> build_properties =
            forward_properties(std::forward<Tags>(tags), tags_to_forward);
        boost::copy(properties_to_export, std::back_inserter(build_properties));
        return Node(std::forward<Point>(p), std::move(build_properties));
    }
};

class WayBuilder {
private:
    std::vector<std::pair<std::string, std::string>> properties_to_export;
    std::vector<std::string> tags_to_forward;

public:
    template <typename EProperties, typename FProperties>
    WayBuilder(EProperties && properties, FProperties && tags_to_forward)
        : properties_to_export(std::forward<EProperties>(properties))
        , tags_to_forward(std::forward<FProperties>(tags_to_forward)) {
        boost::sort(properties_to_export);
        boost::sort(tags_to_forward);
    }

    template <typename Tags, typename Linestring>
    Way build(Tags && tags, Linestring && l) const {
        std::vector<std::pair<std::string, std::string>> build_properties =
            forward_properties(std::forward<Tags>(tags), tags_to_forward);
        boost::copy(properties_to_export, std::back_inserter(build_properties));
        return Way(std::forward<Linestring>(l), std::move(build_properties));
    }
};

class AreaBuilder {
private:
    std::vector<std::pair<std::string, std::string>> properties_to_export;
    std::vector<std::string> tags_to_forward;

public:
    template <typename EProperties, typename FProperties>
    AreaBuilder(EProperties && properties, FProperties && tags_to_forward)
        : properties_to_export(std::forward<EProperties>(properties))
        , tags_to_forward(std::forward<FProperties>(tags_to_forward)) {
        boost::sort(properties_to_export);
        boost::sort(tags_to_forward);
    }

    template <typename Tags, typename Multipolygon>
    Area build(Tags && tags, Multipolygon && mp) const {
        std::vector<std::pair<std::string, std::string>> build_properties =
            forward_properties(std::forward<Tags>(tags), tags_to_forward);
        boost::copy(properties_to_export, std::back_inserter(build_properties));
        return Area(std::forward<Multipolygon>(mp),
                    std::move(build_properties));
    }
};

#endif  // REGION_BUILDERS_HPP