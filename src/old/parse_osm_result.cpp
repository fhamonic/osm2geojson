#include "parse_osm_result.hpp"

#include <iostream>

namespace bg = boost::geometry;
    

static int get_count(const nlohmann::json & count_elem) {
    return std::stoi(count_elem["tags"]["total"].get_ref<const std::string&>());
}

static Linestring way_to_linestring(const nlohmann::json & way) {
    Linestring l;
    for(auto point : way["geometry"])
        l.emplace_back(point.at("lon").get_ref<const double &>(), point.at("lat").get_ref<const double &>());
    return l;
}

static Multipolygon way_to_multipolygon(const nlohmann::json & way) {
    Multipolygon multipolygon;
    multipolygon.resize(1);
    auto & outer = multipolygon[0].outer();
    for(auto point : way["geometry"])
        outer.emplace_back(point.at("lon").get_ref<const double &>(), point.at("lat").get_ref<const double &>());
    return multipolygon;
}

static std::vector<Linestring> relation_to_linestrings(const nlohmann::json & relation) {
    std::vector<Linestring> linestrings;
    linestrings.reserve(relation.size());
    for(auto element : relation["members"]) {
        if(element["type"] != "way")
            continue;
        const Linestring & l = linestrings.emplace_back(way_to_linestring(element));
        if(!bg::is_valid(l)) {
            std::cerr << "Skipping way " << element["id"].get<int>() << " that is not a valid Linestring" << std::endl;
            linestrings.pop_back();
            continue;
        }
    }
    return linestrings;
}

class SphericalPointComparator {
public:
    bool operator()(const SphericalPoint & a, const SphericalPoint & b) const {
        if(a.get<0>() == b.get<0>())
            return a.get<1>() < b.get<1>();
        return a.get<0>() < b.get<0>();
    }
};


static Ring linestring_to_ring(const Linestring & l) {
    Ring ring;
    ring.reserve(l.size());
    ring.insert(ring.end(), l.cbegin(), l.cend());
    return ring;
}

static std::pair<std::vector<int>::const_iterator, std::vector<int>::const_iterator> find_next_linestring(SphericalPoint extremity, const std::vector<std::pair<bool, int>> & linestrings_refs, const std::map<SphericalPoint, std::vector<int>, SphericalPointComparator> & extremity_to_linestrings_ids) {
    auto it = extremity_to_linestrings_ids.find(extremity);
    assert(it != extremity_to_linestrings_ids.cend());
    const std::vector<int> & linestrings_ids = it->second;
    auto first_id = linestrings_ids.cbegin();
    auto end_id = linestrings_ids.cend();
    for(auto first_id = linestrings_ids.cbegin(); first_id != end_id; ++first_id) {
        if(linestrings_refs[*first_id].first)
            continue;
        return std::make_pair(first_id, end_id);
    }
    return std::make_pair(end_id, end_id);
}

static std::vector<Ring> linestrings_to_rings(const std::vector<Linestring> & linestrings) {
    std::vector<Ring> rings;

    std::vector<std::pair<bool, int>> linestrings_refs;
    std::map<SphericalPoint, std::vector<int>, SphericalPointComparator> extremity_to_linestrings_ids;

    for(int i=0; i<linestrings.size(); ++i) {
        const Linestring & l = linestrings[i];
        if(bg::equals(l.front(), l.back())) {
            rings.push_back(linestring_to_ring(l));
            assert(bg::is_valid(rings.back()));
            continue;
        }
        linestrings_refs.emplace_back(false, i);
        const int back_index = linestrings_refs.size()-1;

        extremity_to_linestrings_ids[l.front()].emplace_back(back_index);
        extremity_to_linestrings_ids[l.back()].emplace_back(back_index);
    }

    if(linestrings_refs.empty())
        return rings;

    std::vector<std::pair<std::vector<int>::const_iterator, std::vector<int>::const_iterator>> linestrings_affectations;

    for(auto & [taken, l_id] : linestrings_refs) {
        if(taken) continue;
        taken = true;
        const Linestring & l = linestrings[l_id];
        Ring current_ring;
        current_ring.insert(current_ring.end(), l.begin(), l.end() - 1);

        SphericalPoint extremity = l.back();
        for(;;) {
            while(!bg::equals(extremity, l.front())) {
                auto p = find_next_linestring(extremity, linestrings_refs, extremity_to_linestrings_ids);
                if(p.first == p.second) {
                    assert(false && "Ring assignment failed : No available ways to complete ring");
                }
                const int next_index = *p.first;
                auto & [next_taken, next_id] = linestrings_refs[*p.first];
                next_taken = true;
                const Linestring & next = linestrings[next_id];
                linestrings_affectations.emplace_back(p);

                if(bg::equals(extremity, next.front())) {
                    current_ring.insert(current_ring.end(), next.begin(), next.end() - 1);
                    extremity = next.back();
                } else { assert(bg::equals(extremity, next.back()));
                    current_ring.insert(current_ring.end(), next.rbegin(), next.rend() - 1);
                    extremity = next.front();
                }
            }
            current_ring.emplace_back(extremity);

            if(bg::is_valid(current_ring))
                break;

            std::cout << "backtracking" << std::endl;

            for(auto & p = linestrings_affectations.back(); !linestrings_affectations.empty(); linestrings_affectations.pop_back(), p = linestrings_affectations.back()) {
                int last_linestring_id = *p.first;
                linestrings_refs[last_linestring_id].first = false;
                const Linestring & last = linestrings[linestrings_refs[last_linestring_id].second];
                current_ring.erase(current_ring.end() - (last.size()-1), current_ring.end());
                
                for(++p.first; p.first != p.second; ++p.first) {
                    if(!linestrings_refs[last_linestring_id].first)
                        break;
                }

                if(p.first == p.second) 
                    continue;
                    
                extremity = current_ring.back();
                int new_last_linestring_id = *p.first;
                linestrings_refs[new_last_linestring_id].first = false;
                const Linestring & new_last = linestrings[linestrings_refs[new_last_linestring_id].second];
            
                if(bg::equals(extremity, new_last.front())) {
                    current_ring.insert(current_ring.end(), new_last.begin() - 1, new_last.end() - 1);
                    extremity = new_last.back();
                } else { assert(bg::equals(extremity, new_last.back()));
                    current_ring.insert(current_ring.end(), new_last.rbegin() - 1, new_last.rend() - 1);
                    extremity = new_last.front();
                }

                break;
            }

            if(!linestrings_affectations.empty())
                continue;
            
            assert(false && "Ring assignment failed after backtracking");
        }
    }
    return rings;
}

static std::vector<std::vector<int>> rings_inclusion_tree(const std::vector<Ring> & rings) {
    std::vector<std::vector<int>> inclusion_tree(rings.size()+1);
    const int root = inclusion_tree.size()-1;

    for(int current_ring_id = 0; current_ring_id < rings.size(); ++current_ring_id) {
        const Ring & current_ring = rings[current_ring_id];
        int parent = root;
        auto id_it = inclusion_tree[parent].begin();
        auto id_end = inclusion_tree[parent].end();
        while(id_it != id_end) {
            int & ring_id = *id_it;
            const Ring & ring = rings[ring_id];
            if(bg::within(current_ring, ring)) {
                parent = ring_id;
                id_it = inclusion_tree[parent].begin();
                id_end = inclusion_tree[parent].end();
                continue;
            }
            if(bg::within(ring, current_ring)) {
                inclusion_tree[current_ring_id].emplace_back(ring_id);
                auto id_it_2 = id_it+1;
                auto id_end_2 = id_end;
                while(id_it_2 != id_end_2) {
                    int & ring_id_2 = *id_it_2;
                    const Ring & ring_2 = rings[ring_id_2];
                    if(!bg::within(ring_2, current_ring)) {
                        ++id_it_2;
                        continue;
                    }
                    inclusion_tree[current_ring_id].emplace_back(ring_id_2);
                    ring_id_2 = *(--id_end_2);
                }
                inclusion_tree[parent].resize(std::distance(inclusion_tree[parent].begin(), id_end_2));
                ring_id = current_ring_id;
                break;
            }
            ++id_it;
        }
        if(id_it == id_end)
            inclusion_tree[parent].emplace_back(current_ring_id);
    }

    return inclusion_tree;
}

static Multipolygon rings_to_multipolygon(const std::vector<Ring> & rings) {
    Multipolygon multipolygon;
    const std::vector<std::vector<int>> inclusion_tree = rings_inclusion_tree(rings);

    std::vector<std::pair<std::vector<int>::const_iterator, std::vector<int>::const_iterator>> outer_rings_id_lifo;
    outer_rings_id_lifo.reserve(rings.size());
    outer_rings_id_lifo.emplace_back(inclusion_tree.back().cbegin(), inclusion_tree.back().cend());

    while(!outer_rings_id_lifo.empty()) {
        auto [first, last] = outer_rings_id_lifo.back();
        outer_rings_id_lifo.pop_back();

        for(; first != last; ++first) {
            const int outer_ring_id = *first;
            const Ring & outer_ring = rings[outer_ring_id];

            Polygon & poly = multipolygon.emplace_back();
            poly.outer().resize(outer_ring.size());
            std::copy(outer_ring.cbegin(), outer_ring.cend(), poly.outer().begin());

            poly.inners().reserve(inclusion_tree[outer_ring_id].size());

            for(const int inner_ring_id : inclusion_tree[outer_ring_id]) {
                const Ring & inner_ring = rings[inner_ring_id];
                poly.inners().emplace_back(inner_ring);

                if(inclusion_tree[inner_ring_id].empty())
                    continue;
                
                outer_rings_id_lifo.emplace_back(inclusion_tree[inner_ring_id].cbegin(), inclusion_tree[inner_ring_id].cend());
            }
        }
    }

    return multipolygon;
}

static Multipolygon relation_to_multipolygon(const nlohmann::json & relation) {
    const std::vector<Linestring> linestrings = relation_to_linestrings(relation);
    const std::vector<Ring> rings = linestrings_to_rings(linestrings);
    return rings_to_multipolygon(rings);
}

static Region parse_search_area(const nlohmann::json & element) {
    // Region region;    
    // region.multipolygon.resize(1);
    // region.multipolygon[0].outer().emplace_back(element["bounds"]["minlon"].get_ref<const double &>(),element["bounds"]["minlat"].get_ref<const double &>());
    // region.multipolygon[0].outer().emplace_back(element["bounds"]["maxlon"].get_ref<const double &>(),element["bounds"]["minlat"].get_ref<const double &>());
    // region.multipolygon[0].outer().emplace_back(element["bounds"]["maxlon"].get_ref<const double &>(),element["bounds"]["maxlat"].get_ref<const double &>());
    // region.multipolygon[0].outer().emplace_back(element["bounds"]["minlon"].get_ref<const double &>(),element["bounds"]["maxlat"].get_ref<const double &>());
    // region.multipolygon[0].outer().emplace_back(element["bounds"]["minlon"].get_ref<const double &>(),element["bounds"]["minlat"].get_ref<const double &>());
    // return region;

    const double qualityCoef = element.contains("qualityCoef") ? element.at("qualityCoef").get<double>() : 0;
    const double resistanceCoef = element.contains("resistanceCoef") ? element.at("resistanceCoef").get<double>() : 1000;
    const int overrideLevel = element.contains("overrideLevel") ? element.at("overrideLevel").get<int>() : 0;

    if(element["type"] == "way")
        return Region(way_to_multipolygon(element), qualityCoef, resistanceCoef, overrideLevel);

    if(element["type"] == "relation")
        return Region(relation_to_multipolygon(element), qualityCoef, resistanceCoef, overrideLevel);
    
    assert(false && "Should not be here");
    return Region();
}

static Region parse_area_element_region(const nlohmann::json & areaPattern, const nlohmann::json & element) {
    const double qualityCoef = element.contains("qualityCoef") ? element.at("qualityCoef").get<double>() : 0;
    const double resistanceCoef = element.contains("resistanceCoef") ? element.at("resistanceCoef").get<double>() : 1000;
    const int overrideLevel = element.contains("overrideLevel") ? element.at("overrideLevel").get<int>() : 0;

    if(element["type"] == "way")
        return Region(way_to_multipolygon(element), qualityCoef, resistanceCoef, overrideLevel);

    if(element["type"] == "relation")
        return Region(relation_to_multipolygon(element), qualityCoef, resistanceCoef, overrideLevel);
    
    assert(false && "Should not be here");
    return Region();       
}

static Region parse_way_element_region(const nlohmann::json & wayPattern, const nlohmann::json & way) {
    const double inflate = way.contains("inflatedWidth") ? way.at("inflatedWidth").get<double>() : 1;
    const double qualityCoef = way.contains("qualityCoef") ? way.at("qualityCoef").get<double>() : 0;
    const double resistanceCoef = way.contains("resistanceCoef") ? way.at("resistanceCoef").get<double>() : 1000;
    const int overrideLevel = way.contains("overrideLevel") ? way.at("overrideLevel").get<int>() : 0;

    if(way["type"] != "way") {
        assert(false && "Should not be here");
        return Region();
    }
    const Linestring linestring = way_to_linestring(way);
    Multipolygon multipolygon;

    multipolygon.resize(1);

    return Region(multipolygon, qualityCoef, resistanceCoef, overrideLevel); 
}

void parse_osm_result(const nlohmann::json & query_params, const nlohmann::json & query_result, std::vector<Region> & regions) {
    auto current_element = query_result["elements"].cbegin();
    auto elements_end = query_result["elements"].cend();

    int nb_areas = get_count(*current_element);
    if(nb_areas != 1) {
        std::cerr << "Invalid : searchArea returned " << nb_areas << " areas instead of 1" << std::endl;
        return;
    }
    ++current_element;
    regions.emplace_back(parse_search_area(*current_element));
    ++current_element;
    ++current_element;

    for(auto areaPattern = query_params["areaPatterns"].cbegin();
            areaPattern != query_params["areaPatterns"].cend() && current_element != elements_end;
            ++current_element) {
        if((*current_element)["type"] == "count") {
            ++areaPattern;
            continue;
        }
        regions.emplace_back(parse_area_element_region(*areaPattern, *current_element));
    }
    // for(auto wayPattern = query_params["wayPatterns"].cbegin();
    //         wayPattern != query_params["wayPatterns"].cend() && current_element != elements_end;
    //         ++current_element) {
    //     if((*current_element)["type"] == "count") {
    //         ++wayPattern;
    //         continue;
    //     }
    //     Region & region = regions.emplace_back();
    //     parse_way_element_region(*wayPattern, *current_element, region);
    // }
}

void plot_svg(const std::vector<Region> & regions) {
    std::ofstream svg("my_map.svg");
    boost::geometry::svg_mapper<SphericalPoint> mapper(svg, 100, 100);

    auto first_region = regions.cbegin();
    auto end_regions = regions.cend();


    SphericalPoint min(360, 180);
    SphericalPoint max(-360, -180);
    for(auto region : regions) {
        for(auto poly : region.multipolygon) {
            for(auto p : poly.outer()) {
                min.set<0>(std::min(min.get<0>(), p.get<0>()));
                min.set<1>(std::min(min.get<1>(), p.get<1>()));
                max.set<0>(std::max(max.get<0>(), p.get<0>()));
                max.set<1>(std::max(max.get<1>(), p.get<1>()));
            }
        }
    }

    std::cout << "min : " << min.get<0>() << ", " << min.get<1>() << std::endl;
    std::cout << "max : " << max.get<0>() << ", " << max.get<1>() << std::endl;

    Polygon fp;
    fp.outer().resize(first_region->multipolygon[0].outer().size());
    std::transform(first_region->multipolygon[0].outer().cbegin(), first_region->multipolygon[0].outer().cend(), fp.outer().begin(), [&](const SphericalPoint & p){
        return SphericalPoint((p.get<0>() - min.get<0>()) / (max.get<0>() - min.get<0>()) * 10000,
            (p.get<1>() - min.get<1>()) / (max.get<1>() - min.get<1>()) * 10000);
    });
    for(auto p : fp.outer())
        std::cout << "p: " << p.get<0>() << ", " << p.get<1>() << std::endl;
    
    mapper.add(fp);
    mapper.map(fp, "fill-opacity:0.3;fill:rgb(255,204,255);stroke:rgb(255,0,0);stroke-width:0.1");
    // ++first_region;

    for(; first_region != end_regions; ++first_region) {

        if(std::distance(first_region, end_regions) == 2) break;

        Polygon poly;
        poly.outer().resize(first_region->multipolygon[0].outer().size());
        std::transform(first_region->multipolygon[0].outer().cbegin(), first_region->multipolygon[0].outer().cend(), poly.outer().begin(), [&](const SphericalPoint & p){
            return SphericalPoint((p.get<0>() - min.get<0>()) / (max.get<0>() - min.get<0>()) * 10000,
                (p.get<1>() - min.get<1>()) / (max.get<1>() - min.get<1>()) * 10000);
        });
        for(auto p : poly.outer())
            std::cout << std::distance(first_region, end_regions) << " p: " << p.get<0>() << ", " << p.get<1>() << std::endl;
        mapper.add(poly);
        mapper.map(poly, "fill-opacity:0.3;fill:rgb(0,154,0);stroke:rgb(0,255,0);stroke-width:0.1");
    }
}