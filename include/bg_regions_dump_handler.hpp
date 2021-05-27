#ifndef BG_REGIONS_DUMP_HANDLER_HPP
#define BG_REGIONS_DUMP_HANDLER_HPP

#include <iostream>
#include <utility>
#include <vector>

#include <osmium/handler.hpp>
#include <osmium/tags/tags_filter.hpp>

#include "bg_types.hpp"
#include "bg_factory.hpp"
#include "region.hpp"
#include "region_builders.hpp"

#include <boost/log/trivial.hpp>

class BGRegionsDumpHandler : public osmium::handler::Handler {
private:
    osmium::geom::BGFactory m_factory;
    
    MultipolygonGeo search_area;
    BoxGeo search_area_box;

    bool fusion_test(const std::vector<std::pair<std::string, std::string>> & tags, const std::vector<std::pair<std::string, std::string>> & filter) {
        auto tags_first = tags.cbegin(); 
        auto tags_last = tags.cend(); 
        auto filter_first = filter.cbegin(); 
        auto filter_last = filter.cend();
        while(tags_first != tags_last && filter_first != filter_last) {
            int r_cmp = tags_first->first.compare(filter_first->first);
            if(r_cmp < 0) { ++tags_first; continue; }
            if(r_cmp > 0) 
                return false;
            if(filter_first->second.compare(tags_first->second) != 0)
                return false;
            ++tags_first; ++filter_first;
        }
        return filter_first == filter_last;
    }

    std::vector<std::pair<std::vector<std::pair<std::string,std::string>>, NodeRegionBuilder>> node_filters;
    std::vector<std::pair<std::vector<std::pair<std::string,std::string>>, WayRegionBuilder>> way_filters;
    std::vector<std::pair<std::vector<std::pair<std::string,std::string>>, AreaRegionBuilder>> area_filters;

    std::vector<Region> regions;

public:
    template <typename NFilters, typename WFilters, typename AFilters>
    BGRegionsDumpHandler(NFilters&& node_filters, WFilters&& way_filters, AFilters&& area_filters)
            : osmium::handler::Handler()
            , search_area_box(boost::geometry::make_inverse<BoxGeo>())
            , node_filters(std::forward<NFilters>(node_filters))
            , way_filters(std::forward<WFilters>(way_filters)) 
            , area_filters(std::forward<AFilters>(area_filters))
            {}

    template <typename SArea, typename NFilters, typename WFilters, typename AFilters>
    BGRegionsDumpHandler(SArea&& p_search_area, NFilters&& node_filters, WFilters&& way_filters, AFilters&& area_filters)
            : osmium::handler::Handler()
            , search_area(p_search_area)
            , search_area_box(boost::geometry::return_envelope<BoxGeo>(search_area))
            , node_filters(std::forward<NFilters>(node_filters))
            , way_filters(std::forward<WFilters>(way_filters)) 
            , area_filters(std::forward<AFilters>(area_filters))
            {}

    template <typename NFilter, typename ... NInfoArgs>
    void add_node_rule(NFilter&& filter, NInfoArgs&&... args) const noexcept {
        node_filters.emplace_back(std::piecewise_construct, 
                std::forward_as_tuple(filter),
                std::forward_as_tuple(args...));
    }
    template <typename WFilter, typename ... WInfoArgs>
    void add_way_rule(WFilter&& filter, WInfoArgs&&... args) const noexcept {
        way_filters.emplace_back(std::piecewise_construct, 
                std::forward_as_tuple(filter),
                std::forward_as_tuple(args...));
    }
    template <typename AFilter, typename ... AInfoArgs>
    void add_area_rule(AFilter&& filter, AInfoArgs&&... args) const noexcept {
        area_filters.emplace_back(std::piecewise_construct, 
                std::forward_as_tuple(filter),
                std::forward_as_tuple(args...));
    }

    template <typename SearchArea>
    void setSearchArea(SearchArea&& area) noexcept { search_area = std::forward<SearchArea>(area); }

    const BoxGeo & getSearchBox() const noexcept { return search_area_box; }
    const MultipolygonGeo & getSearchArea() const noexcept { return search_area; }

    const std::vector<Region> & getRegions() const noexcept { return regions; }

    void node(const osmium::Node& node) noexcept {
        try {
            if(!boost::geometry::covered_by(m_factory.create_point(node), search_area_box))
                return;

            std::vector<std::pair<std::string, std::string>> tags;
            tags.reserve(node.tags().size());
            boost::transform(node.tags(), std::back_inserter(tags), [](const osmium::Tag & tag) { return std::make_pair(tag.key(), tag.value()); });
            boost::sort(tags, [](const auto & p1, const auto & p2){ return p1.first < p2.first; });

            for(auto & [filter, builder] : node_filters) {
                if(!fusion_test(tags, filter)) continue;

                regions.emplace_back(builder.build(std::move(tags), m_factory.create_point(node)));
                if(search_area.empty() || boost::geometry::covered_by(regions.back().multipolygon, search_area))
                    return;
                regions.pop_back();
                return;
            }
        } catch (const osmium::geometry_error& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        } catch (const osmium::invalid_location& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        }
    }
    void way(const osmium::Way& way) noexcept {
        try {
            if(!search_area.empty() && !boost::geometry::intersects(m_factory.envelope(way), search_area_box))
                return;

            std::vector<std::pair<std::string, std::string>> tags;
            boost::transform(way.tags(), std::back_inserter(tags), [](const osmium::Tag & tag) { return std::make_pair(tag.key(), tag.value()); });
            boost::sort(tags, [](const auto & p1, const auto & p2){ return p1.first < p2.first; });

            for(auto & [filter, builder] : way_filters) {
                if(!fusion_test(tags, filter)) continue;

                regions.emplace_back(builder.build(tags, m_factory.create_linestring(way)));
                if(search_area.empty() || boost::geometry::covered_by(regions.back().multipolygon, search_area))
                    return;
                regions.pop_back();
                return;
            }
        } catch (const osmium::geometry_error& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        } catch (const osmium::invalid_location& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        }
    }    
    void area(const osmium::Area& area) noexcept {
        try {
            if(!search_area.empty() && !boost::geometry::intersects(m_factory.envelope(area), search_area_box))
                return;

            std::vector<std::pair<std::string, std::string>> tags;
            tags.reserve(area.tags().size());
            boost::transform(area.tags(), std::back_inserter(tags), [](const osmium::Tag & tag) { return std::make_pair(tag.key(), tag.value()); });
            boost::sort(tags, [](const auto & p1, const auto & p2){ return p1.first < p2.first; });

            for(auto & [filter, builder] : area_filters) {
                if(!fusion_test(tags, filter)) continue;

                regions.emplace_back(builder.build(tags, m_factory.create_multipolygon(area)));
                if(search_area.empty() || boost::geometry::intersects(regions.back().multipolygon, search_area))
                    return;
                regions.pop_back();
                return;
            }
        } catch (const osmium::geometry_error& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        } catch (const osmium::invalid_location& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        }
    }
}; // class BGRegionsDumpHandler

#endif // BG_REGIONS_DUMP_HANDLER_HPP