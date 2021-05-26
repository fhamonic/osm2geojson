#ifndef BG_TYPES_HPP
#define BG_TYPES_HPP 

#include <boost/geometry.hpp>

using PointGeo = boost::geometry::model::d2::point_xy<double, boost::geometry::cs::geographic<boost::geometry::degree>>;
using BoxGeo = boost::geometry::model::box<PointGeo>;
using LinestringGeo = boost::geometry::model::linestring<PointGeo>;
using RingGeo = boost::geometry::model::ring<PointGeo>;
using PolygonGeo = boost::geometry::model::polygon<PointGeo>;
using MultipolygonGeo = boost::geometry::model::multi_polygon<PolygonGeo>;

using Point2D = boost::geometry::model::d2::point_xy<double>;
using Box2D = boost::geometry::model::box<Point2D>;
using Linestring2D = boost::geometry::model::linestring<Point2D>;
using Ring2D = boost::geometry::model::ring<Point2D>;
using Polygon2D = boost::geometry::model::polygon<Point2D>;
using Multipolygon2D = boost::geometry::model::multi_polygon<Polygon2D>;

#endif // BG_TYPES_HPP