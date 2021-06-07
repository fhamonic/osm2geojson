#ifndef BG_UTILS_HPP
#define BG_UTILS_HPP 

#include "bg_types.hpp"

#include <boost/geometry/srs/epsg.hpp>
#include <boost/geometry/srs/projection.hpp>
#include <boost/geometry/srs/projections/proj4.hpp>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>

template <int ppc, class T>
Multipolygon2D buffer2D(T&& g, float width) {
    Multipolygon2D mp2d;
    boost::geometry::strategy::buffer::distance_symmetric<float> distance_strategy(width / 2);
    boost::geometry::strategy::buffer::join_round join_strategy(ppc);
    boost::geometry::strategy::buffer::end_flat end_strategy;
    boost::geometry::strategy::buffer::point_circle point_strategy(ppc);
    boost::geometry::strategy::buffer::side_straight side_strategy;
    boost::geometry::buffer(g, mp2d, distance_strategy, side_strategy, join_strategy, end_strategy, point_strategy);
    return mp2d;
}

template <class Point>
MultipolygonGeo buffer_PointGeo(Point&& p, float width) {
    boost::geometry::srs::projection<> proj = boost::geometry::srs::proj4("+proj=eqc +ellps=GRS80 +lon_0="+ std::to_string(p.x()) +" +lat_0="+ std::to_string(p.y()));
     
    MultipolygonGeo mp;

    Point2D p2d;
    proj.forward(p, p2d);
    Multipolygon2D mp2d = buffer2D<8>(p2d, width);
    proj.inverse(mp2d, mp);

    return mp;
}

template <class Linestring>
MultipolygonGeo buffer_LinestringGeo(Linestring&& l, float width) {
    PointGeo center = boost::geometry::return_centroid<PointGeo>(boost::geometry::return_envelope<BoxGeo>(l));
    boost::geometry::srs::projection<> proj = boost::geometry::srs::proj4("+proj=eqc +ellps=GRS80 +lon_0="+ std::to_string(center.x()) +" +lat_0="+ std::to_string(center.y()));
     
    MultipolygonGeo mp;

    Linestring2D l2d;
    proj.forward(l, l2d);

    Linestring2D l2d_simplified;
    boost::geometry::simplify(l2d, l2d_simplified, 0.5);

    
    l2d_simplified[0] = Point2D((9999*l2d_simplified[0].get<0>() + l2d_simplified[1].get<0>())/10000,
        (9999*l2d_simplified[0].get<1>() + l2d_simplified[1].get<1>())/10000);

    const int last_id = l2d_simplified.size()-1;
    l2d_simplified[last_id] = Point2D((9999*l2d_simplified[last_id].get<0>() + l2d_simplified[last_id-1].get<0>())/10000,
        (9999*l2d_simplified[last_id].get<1>() + l2d_simplified[last_id-1].get<1>())/10000);

    
    Multipolygon2D mp2d = buffer2D<4>(l2d_simplified, width);

    Multipolygon2D mp2d_simplified;
    boost::geometry::simplify(mp2d, mp2d_simplified, 0.5);

    proj.inverse(mp2d_simplified, mp);

    if(! boost::geometry::is_valid(mp)) {
        std::string m;
        boost::geometry::is_valid(mp, m);
        throw std::runtime_error{m};
    }

    return mp;
}

#endif // BG_UTILS_HPP