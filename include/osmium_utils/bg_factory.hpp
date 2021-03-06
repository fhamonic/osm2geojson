#ifndef OSMIUM_BG_FACTORY_HPP
#define OSMIUM_BG_FACTORY_HPP

/*

This file is part of Osmium (https://osmcode.org/libosmium).

Copyright 2013-2021 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <osmium/geom/coordinates.hpp>
#include <osmium/memory/collection.hpp>
#include <osmium/memory/item.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/node_ref_list.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/osm/way.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

#include <osmium/geom/factory.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/multi_polygon.hpp>

namespace osmium {
    namespace geom {
        /**
         * Boost Geometry factory.
         */
        class BGFactory {

        public:
            using point_type        = boost::geometry::model::d2::point_xy<double, boost::geometry::cs::geographic<boost::geometry::degree>>;
            using box_type          = boost::geometry::model::box<point_type>;
            using linestring_type   = boost::geometry::model::linestring<point_type>;
            using polygon_type      = boost::geometry::model::polygon<point_type>;
            using multipolygon_type = boost::geometry::model::multi_polygon<polygon_type>;
            using ring_type         = boost::geometry::model::ring<point_type>;

        public:
            BGFactory() { }
            BGFactory(const BGFactory &) = default;
            BGFactory(BGFactory&&) = default;
            ~BGFactory() = default;
            BGFactory& operator=(const BGFactory &) = default;
            BGFactory& operator=(BGFactory&&) = default;

            int epsg() const noexcept { return 4326; }
            std::string proj_string() const { return "+proj=longlat +datum=WGS84 +no_defs"; }

            /* Point */

            point_type create_point(const osmium::Location& location) const {
                return point_type(location.lon(), location.lat());
            }

            point_type create_point(const osmium::Node& node) const {
                try {
                    return create_point(node.location());
                } catch (osmium::geometry_error& e) {
                    e.set_id("node", node.id());
                    throw;
                }
            }

            point_type create_point(const osmium::NodeRef& node_ref) const {
                try {
                    return create_point(node_ref.location());
                } catch (osmium::geometry_error& e) {
                    e.set_id("node", node_ref.ref());
                    throw;
                }
            }

            /* LineString */

            linestring_type create_linestring(const osmium::WayNodeList& wnl, use_nodes un = use_nodes::unique, direction dir = direction::forward) const {
                linestring_type l;

                l.resize(wnl.size());
                if(dir == direction::forward)
                    std::transform(wnl.cbegin(), wnl.cend(), l.begin(), [&](const osmium::NodeRef& n){ return create_point(n); });
                else
                    std::transform(wnl.crbegin(), wnl.crend(), l.begin(), [&](const osmium::NodeRef& n){ return create_point(n); });
                
                if(un == use_nodes::unique)
                    boost::geometry::unique(l);

                if(l.size() < 2)
                    throw osmium::geometry_error{"need at least two points for linestring"};

                if(! boost::geometry::is_valid(l)) {
                    std::string m;
                    boost::geometry::is_valid(l, m);
                    throw osmium::geometry_error{m};
                }

                return l;
            }

            linestring_type create_linestring(const osmium::Way& way, use_nodes un = use_nodes::unique, direction dir = direction::forward) const {
                try {
                    return create_linestring(way.nodes(), un, dir);
                } catch (osmium::geometry_error& e) {
                    e.set_id("way", way.id());
                    throw;
                }
            }

            /* Polygon */

            polygon_type create_polygon(const osmium::WayNodeList& wnl, use_nodes un = use_nodes::unique, direction dir = direction::forward) const {
                polygon_type p;

                p.outer().resize(wnl.size());
                if(dir == direction::forward)
                    std::transform(wnl.cbegin(), wnl.cend(), p.outer().begin(), [&](const osmium::NodeRef& n){ return create_point(n); });
                else
                    std::transform(wnl.crbegin(), wnl.crend(), p.outer().begin(), [&](const osmium::NodeRef& n){ return create_point(n); });
                
                if (un == use_nodes::unique)
                    boost::geometry::unique(p);

                if (p.outer().size() < 4)
                    throw osmium::geometry_error{"need at least four points for polygon"};

                if (! boost::geometry::is_valid(p)) {
                    std::string m;
                    boost::geometry::is_valid(p, m);
                    throw osmium::geometry_error{m};
                }
                
                return p;
            }

            polygon_type create_polygon(const osmium::Way& way, use_nodes un = use_nodes::unique, direction dir = direction::forward) const {
                try {
                    return create_polygon(way.nodes(), un, dir);
                } catch (osmium::geometry_error& e) {
                    e.set_id("way", way.id());
                    throw;
                }
            }

            /* MultiPolygon */

            multipolygon_type create_multipolygon(const osmium::Area& area) const {
                try {
                    multipolygon_type mp;
                    polygon_type * current_polygon = nullptr;
                    for (const auto& item : area) {
                        if (item.type() == osmium::item_type::outer_ring) {
                            const auto& ring = static_cast<const osmium::OuterRing&>(item);
                            current_polygon = &mp.emplace_back();
                            current_polygon->outer().resize(ring.size());
                            std::transform(ring.crbegin(), ring.crend(), current_polygon->outer().begin(), 
                                    [&](const osmium::NodeRef& n){ return create_point(n); });
                        } else if (item.type() == osmium::item_type::inner_ring) {
                            const auto& ring = static_cast<const osmium::InnerRing&>(item);
                            ring_type & current_ring = current_polygon->inners().emplace_back();
                            current_ring.resize(ring.size());
                            std::transform(ring.crbegin(), ring.crend(), current_ring.begin(), 
                                    [&](const osmium::NodeRef& n){ return create_point(n); });
                        }
                    }

                    if(mp.empty())
                        throw osmium::geometry_error{"invalid area"};

                    if(!boost::geometry::is_valid(mp)) {
                        std::string m;
                        boost::geometry::is_valid(mp, m);
                        throw osmium::geometry_error{m};
                    }

                    return mp;
                } catch (osmium::geometry_error& e) {
                    e.set_id("area", area.id());
                    throw;
                }
            }

            template<class T>
            box_type envelope(T&& elem) {
                const osmium::Box box = elem.envelope();
                return box_type(create_point(box.bottom_left()), create_point(box.top_right()));
            }
        }; // class BGFactory
    } // namespace geom
} // namespace osmium

#endif // OSMIUM_GEOM_FACTORY_HPP
