#ifndef BG_DUMP_HANDLER_HPP
#define BG_DUMP_HANDLER_HPP

#include <iostream>
#include <utility>
#include <vector>

#include <osmium/handler.hpp>
#include <osmium/tags/tags_filter.hpp>

#include "osmium_utils/bg_factory.hpp"

#include "bg_types.hpp"
#include "builders.hpp"

#include <boost/log/trivial.hpp>

#include <iterator>

class BGDumpHandler : public osmium::handler::Handler {
private:
    osmium::geom::BGFactory m_factory;

    MultipolygonGeo search_area;
    BoxGeo search_area_box;

    template <typename Tags>
    bool fusion_test(
        Tags && tags,
        const std::vector<std::pair<std::string, osmium::StringMatcher>> &
            filter) {
        auto tags_first = tags.cbegin();
        auto tags_last = tags.cend();
        auto filter_first = filter.cbegin();
        auto filter_last = filter.cend();
        while(tags_first != tags_last && filter_first != filter_last) {
            int r_cmp = tags_first->first.compare(filter_first->first);
            if(r_cmp < 0) {
                ++tags_first;
                continue;
            }
            if(r_cmp > 0) return false;
            if(!filter_first->second(tags_first->second.data())) return false;
            ++tags_first;
            ++filter_first;
        }
        return filter_first == filter_last;
    }

    template <typename Tags>
    std::vector<std::pair<std::string_view, std::string_view>>
    get_sorted_tag_views(Tags && tags) {
        std::vector<std::pair<std::string_view, std::string_view>> tags_views;
        tags_views.reserve(tags.size());
        std::transform(tags.cbegin(), tags.cend(),
                       std::back_inserter(tags_views),
                       [](const osmium::Tag & tag) {
                           return std::make_pair(tag.key(), tag.value());
                       });
        std::sort(tags_views.begin(), tags_views.end(),
                  [](const auto & p1, const auto & p2) {
                      return p1.first < p2.first;
                  });
        return tags_views;
    }

    std::vector<
        std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
                  NodeBuilder>>
        node_filters;
    std::vector<std::pair<
        std::vector<std::pair<std::string, osmium::StringMatcher>>, WayBuilder>>
        way_filters;
    std::vector<
        std::pair<std::vector<std::pair<std::string, osmium::StringMatcher>>,
                  AreaBuilder>>
        area_filters;

    std::vector<Node> nodes;
    std::vector<Way> ways;
    std::vector<Area> areas;

public:
    template <typename NFilters, typename WFilters, typename AFilters>
    BGDumpHandler(NFilters && node_filters, WFilters && way_filters,
                  AFilters && area_filters)
        : osmium::handler::Handler()
        , search_area_box(boost::geometry::make_inverse<BoxGeo>())
        , node_filters(std::forward<NFilters>(node_filters))
        , way_filters(std::forward<WFilters>(way_filters))
        , area_filters(std::forward<AFilters>(area_filters)) {}

    template <typename SArea, typename NFilters, typename WFilters,
              typename AFilters>
    BGDumpHandler(SArea && p_search_area, NFilters && node_filters,
                  WFilters && way_filters, AFilters && area_filters)
        : osmium::handler::Handler()
        , search_area(p_search_area)
        , search_area_box(boost::geometry::return_envelope<BoxGeo>(search_area))
        , node_filters(std::forward<NFilters>(node_filters))
        , way_filters(std::forward<WFilters>(way_filters))
        , area_filters(std::forward<AFilters>(area_filters)) {}

    template <typename NFilter, typename... NInfoArgs>
    void add_node_rule(NFilter && filter, NInfoArgs &&... args) const noexcept {
        node_filters.emplace_back(std::piecewise_construct,
                                  std::forward_as_tuple(filter),
                                  std::forward_as_tuple(args...));
    }
    template <typename WFilter, typename... WInfoArgs>
    void add_way_rule(WFilter && filter, WInfoArgs &&... args) const noexcept {
        way_filters.emplace_back(std::piecewise_construct,
                                 std::forward_as_tuple(filter),
                                 std::forward_as_tuple(args...));
    }
    template <typename AFilter, typename... AInfoArgs>
    void add_area_rule(AFilter && filter, AInfoArgs &&... args) const noexcept {
        area_filters.emplace_back(std::piecewise_construct,
                                  std::forward_as_tuple(filter),
                                  std::forward_as_tuple(args...));
    }

    template <typename SearchArea>
    void setSearchArea(SearchArea && area) noexcept {
        search_area = std::forward<SearchArea>(area);
    }

    const BoxGeo & getSearchBox() const noexcept { return search_area_box; }
    const MultipolygonGeo & getSearchArea() const noexcept {
        return search_area;
    }
    osmium::TagsFilter getAreaEnglobingFilter() const noexcept {
        osmium::TagsFilter filter{false};
        for(const auto & rule : area_filters)
            for(const auto & [tag, value] : rule.first)
                filter.add_rule(true, tag, value);
        return filter;
    }

    const std::vector<Node> & getNodes() const noexcept { return nodes; }
    const std::vector<Way> & getWays() const noexcept { return ways; }
    const std::vector<Area> & getAreas() const noexcept { return areas; }

    void node(const osmium::Node & node) noexcept {
        try {
            if(!boost::geometry::covered_by(m_factory.create_point(node),
                                            search_area_box))
                return;

            std::vector<std::pair<std::string_view, std::string_view>> tags =
                get_sorted_tag_views(node.tags());

            for(auto & [filter, builder] : node_filters) {
                if(!fusion_test(tags, filter)) continue;

                PointGeo p = m_factory.create_point(node);
                if(!(search_area.empty() ||
                     boost::geometry::intersects(p, search_area)))
                    return;
                nodes.emplace_back(builder.build(tags, std::move(p)));
                return;
            }
        } catch(const osmium::geometry_error & e) {
            BOOST_LOG_TRIVIAL(warning)
                << "Discarded OSM entity: " << e.what() << std::endl;
        } catch(const osmium::invalid_location & e) {
            BOOST_LOG_TRIVIAL(warning)
                << "Discarded OSM entity: " << e.what() << std::endl;
        } catch(const std::runtime_error & e) {
            BOOST_LOG_TRIVIAL(warning)
                << "Discarded OSM entity: " << e.what() << std::endl;
        }
    }
    void way(const osmium::Way & way) noexcept {
        try {
            if(!search_area.empty() &&
               !boost::geometry::intersects(m_factory.envelope(way),
                                            search_area_box))
                return;

            std::vector<std::pair<std::string_view, std::string_view>> tags =
                get_sorted_tag_views(way.tags());

            for(auto & [filter, builder] : way_filters) {
                if(!fusion_test(tags, filter)) continue;

                LinestringGeo l = m_factory.create_linestring(way);
                if(!(search_area.empty() ||
                     boost::geometry::covered_by(l, search_area)))
                    return;
                ways.emplace_back(builder.build(tags, std::move(l)));
                return;
            }
        } catch(const osmium::geometry_error & e) {
            BOOST_LOG_TRIVIAL(warning)
                << "Discarded OSM entity: " << e.what() << std::endl;
        } catch(const osmium::invalid_location & e) {
            BOOST_LOG_TRIVIAL(warning)
                << "Discarded OSM entity: " << e.what() << std::endl;
        } catch(const std::runtime_error & e) {
            BOOST_LOG_TRIVIAL(warning)
                << "Discarded OSM entity: " << e.what() << std::endl;
        }
    }
    void area(const osmium::Area & area) noexcept {
        try {
            if(!search_area.empty() &&
               !boost::geometry::intersects(m_factory.envelope(area),
                                            search_area_box))
                return;

            std::vector<std::pair<std::string_view, std::string_view>> tags =
                get_sorted_tag_views(area.tags());

            for(auto & [filter, builder] : area_filters) {
                if(!fusion_test(tags, filter)) continue;

                MultipolygonGeo mp = m_factory.create_multipolygon(area);
                if(!(search_area.empty() ||
                     boost::geometry::intersects(mp, search_area)))
                    return;
                areas.emplace_back(builder.build(tags, std::move(mp)));
                return;
            }
        } catch(const osmium::geometry_error & e) {
            BOOST_LOG_TRIVIAL(warning)
                << "Discarded OSM entity: " << e.what() << std::endl;
        } catch(const osmium::invalid_location & e) {
            BOOST_LOG_TRIVIAL(warning)
                << "Discarded OSM entity: " << e.what() << std::endl;
        } catch(const std::runtime_error & e) {
            BOOST_LOG_TRIVIAL(warning)
                << "Discarded OSM entity: " << e.what() << std::endl;
        }
    }
};  // class BGDumpHandler

#endif  // BG_DUMP_HANDLER_HPP