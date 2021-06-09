#ifndef BG_H3_INTERFACE_HPP
#define BG_H3_INTERFACE_HPP

#include <h3/h3api.h>
#include <boost/geometry.hpp>

#include "bg_types.hpp"

GeoCoord create_geocoord(const PointGeo & p) noexcept;
PointGeo create_point(const GeoCoord & c) noexcept;

Geofence create_outer_geofence(const RingGeo & r) noexcept;
Geofence create_inner_geofence(const RingGeo & r) noexcept;
void free_geofence(Geofence & fence) noexcept;

GeoPolygon create_geopolygon(const PolygonGeo & p) noexcept;
void free_geopolygon(GeoPolygon & poly) noexcept;

RingGeo create_ring_from_boundary(const GeoBoundary & boundary) noexcept;
RingGeo create_ring_from_fence(const Geofence & fence) noexcept;
PolygonGeo create_polygon(const GeoPolygon poly) noexcept;

PointGeo indexToCenter(H3Index index) noexcept;
RingGeo indexToRing(H3Index index) noexcept;

std::vector<H3Index> polyfill(const PolygonGeo & hull, int res) noexcept;

std::array<H3Index,6> indexToNeighbors(H3Index index);

#endif // BG_H3_INTERFACE_HPP