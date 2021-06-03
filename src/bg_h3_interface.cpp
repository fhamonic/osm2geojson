#include "bg_h3_interface.hpp"


GeoCoord create_geocoord(const PointGeo & p) noexcept {
    return GeoCoord{degsToRads(p.y()), degsToRads(p.x())};
}
PointGeo create_point(const GeoCoord & c) noexcept {
    return PointGeo(radsToDegs(c.lon), radsToDegs(c.lat));
}

Geofence create_outer_geofence(const RingGeo & r) noexcept {
    Geofence fence{static_cast<int>(r.size()-1), new GeoCoord[r.size()-1]};
    std::transform(r.crbegin(), r.crend()-1, fence.verts, create_geocoord);
    return fence;
}
Geofence create_inner_geofence(const RingGeo & r) noexcept {
    Geofence fence{static_cast<int>(r.size()-1), new GeoCoord[r.size()-1]};
    std::transform(r.cbegin(), r.cend()-1, fence.verts, create_geocoord);
    return fence;
}
void free_geofence(Geofence & fence) noexcept {
    delete[] fence.verts;
    fence.numVerts = 0;
    fence.verts = nullptr;
}

GeoPolygon create_geopolygon(const PolygonGeo & p) noexcept {
    GeoPolygon poly{create_outer_geofence(p.outer()), static_cast<int>(p.inners().size()), nullptr};
    if(p.inners().size()) {
        poly.holes = new Geofence[p.inners().size()];
        std::transform(p.inners().cbegin(), p.inners().cend(), poly.holes, create_inner_geofence);
    }
    return poly;
}
void free_geopolygon(GeoPolygon & poly) noexcept {
    free_geofence(poly.geofence);
    std::for_each(poly.holes, poly.holes + poly.numHoles, free_geofence);
    delete[] poly.holes;
    poly.numHoles = 0;
    poly.holes = nullptr;
}


RingGeo create_ring_from_boundary(const GeoBoundary & boundary) noexcept {
    RingGeo r;
    r.reserve(MAX_CELL_BNDRY_VERTS);
    std::transform(std::reverse_iterator(boundary.verts + boundary.numVerts), std::reverse_iterator(boundary.verts), std::back_insert_iterator(r), create_point);
    r.push_back(r.front());
    return r;
}
RingGeo create_ring_from_fence(const Geofence & fence) noexcept {
    RingGeo r;
    std::transform(std::reverse_iterator(fence.verts + fence.numVerts), std::reverse_iterator(fence.verts), std::back_insert_iterator(r), create_point);
    r.push_back(r.front());
    return r;
}
PolygonGeo create_polygon(const GeoPolygon poly) noexcept {
    PolygonGeo p;
    p.outer() = create_ring_from_fence(poly.geofence);
    p.inners().reserve(poly.numHoles);
    std::transform(poly.holes, poly.holes + poly.numHoles, std::back_inserter(p.inners()), create_ring_from_fence);
    return p;
}

PointGeo indexToCenter(H3Index index) noexcept {
    GeoCoord coords;
    h3ToGeo(index, &coords);
    return create_point(coords);
}
RingGeo indexToRing(H3Index index) noexcept {
    GeoBoundary boundary;
    h3ToGeoBoundary(index, &boundary);
    return create_ring_from_boundary(boundary);
}

std::vector<H3Index> polyfill(const PolygonGeo & hull, int res) noexcept {
    GeoPolygon geo_hull = create_geopolygon(hull);
    const int max_size = maxPolyfillSize(&geo_hull, res);
    std::vector<H3Index> hex_indices(max_size, 0);
    polyfill(&geo_hull, res, hex_indices.data());
    hex_indices.resize(std::distance(hex_indices.begin(), std::remove(hex_indices.begin(), hex_indices.end(), 0)));
    free_geopolygon(geo_hull);
    return hex_indices;
}

std::array<H3Index,6> indexToNeighbors(H3Index index) {
    std::array<H3Index, 6> hex_indices;
    if(hexRing(index, 1, hex_indices.data()))
        throw std::runtime_error("fucking pentagon");
    return hex_indices;
}