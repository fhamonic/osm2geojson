#ifndef BG_REGIONS_DUMP_HANDLER_HPP
#define BG_REGIONS_DUMP_HANDLER_HPP

#include <iostream>
#include <utility>
#include <vector>

#include <osmium/handler.hpp>
#include <osmium/tags/tags_filter.hpp>

#include "osmium_utils/bg_factory.hpp"

#include "bg_types.hpp"
#include "region.hpp"
#include "region_builders.hpp"

#include <boost/log/trivial.hpp>

#include <iterator>

class BGRegionsDumpHandler : public osmium::handler::Handler {
private:
    osmium::geom::BGFactory m_factory;

    osmium::TagsFilter node_filter;
    osmium::TagsFilter way_filter;
    osmium::TagsFilter area_filter;

    std::vector<PointGeo> nodes;
    std::vector<LinestringGeo> ways;
    std::vector<MultipolygonGeo> areas;

public:
    template <typename NFilters, typename WFilters, typename AFilters>
    BGRegionsDumpHandler(NFilters&& node_filters, WFilters&& way_filters, AFilters&& area_filters)
            : osmium::handler::Handler()
            , node_filters(std::forward<NFilters>(node_filters))
            , way_filters(std::forward<WFilters>(way_filters)) 
            , area_filters(std::forward<AFilters>(area_filters))
            {}

    template <typename NFilter>
    void add_node_rule(NFilter&& filter) const noexcept {
        node_filters.emplace_back(std::forward<NFilter>(filter));
    }
    template <typename WFilter>
    void add_way_rule(WFilter&& filter) const noexcept {
        way_filters.emplace_back(std::forward<WFilter>(filter));
    }
    template <typename AFilter>
    void add_area_rule(AFilter&& filter) const noexcept {
        area_filters.emplace_back(std::forward<AFilter>(filter));
    }

    const osmium::TagsFilter & getAreaEnglobingFilter() const noexcept {
        return area_filter;
    }

    const std::vector<PointGeo> & getNodes() const noexcept { return nodes; }
    const std::vector<LinestringGeo> & getWays() const noexcept { return ways; }
    const std::vector<MultipolygonGeo> & getAreas() const noexcept { return areas; }

    void node(const osmium::Node& node) noexcept {
        try {
            if(std::none_of(node.tags().begin(), node.tags().end(), node_filter))
                return;
            nodes.emplace_back(m_factory.create_point(node));
        } catch (const osmium::geometry_error& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        } catch (const osmium::invalid_location& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        } catch (const std::runtime_error& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        }
    }
    void way(const osmium::Way& way) noexcept {
        try {
            if(std::none_of(way.tags().begin(), way.tags().end(), way_filter))
                return;
            ways.emplace_back(m_factory.create_linestring(way));
        } catch (const osmium::geometry_error& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        } catch (const osmium::invalid_location& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        } catch (const std::runtime_error& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        }
    }    
    void area(const osmium::Area& area) noexcept {
        try {
            if(std::none_of(area.tags().begin(), area.tags().end(), area_filter))
                return;
            areas.emplace_back(m_factory.create_multipolygon(area));
        } catch (const osmium::geometry_error& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        } catch (const osmium::invalid_location& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        } catch (const std::runtime_error& e) {
            BOOST_LOG_TRIVIAL(warning) << "Discarded OSM entity: " << e.what() << std::endl;
        }
    }
}; // class BGRegionsDumpHandler

#endif // BG_REGIONS_DUMP_HANDLER_HPP