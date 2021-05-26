#include "osm_query.hpp"

static void add_filters(const nlohmann::json & pattern, std::stringstream & ss) {
    if(pattern.contains("matchPropertiesValues"))
        for(auto & [tag, value] : pattern["matchPropertiesValues"].items()) 
            ss << "[" << tag << "=" << value << "]";
    if(pattern.contains("matchPropertiesRegexp"))
        for(auto & [tag, value] : pattern["matchPropertiesRegexp"].items()) 
            ss << "[" << tag << "~" << value << "]";

    if(pattern.contains("requireProperties"))
        for(auto & tag : pattern["requireProperties"]) 
            ss << "[" << tag << "]";
    if(pattern.contains("excludeProperties"))
        for(auto & tag : pattern["excludeProperties"]) 
            ss << "[!" << tag << "]";
}

int osm_build_query(const nlohmann::json & query_params, std::string & query) {
    std::stringstream ss;
    int set_num = 0;

    ss << "[out:json];" << "area";
    add_filters(query_params.at("searchArea"), ss);
    ss << "->.a;way(pivot.a)->.sa;rel(pivot.a)->.sa;";
    for(auto areaPattern : query_params["areaPatterns"]) {
        ss << "(way(area.a)"; add_filters(areaPattern, ss); ss << ";";
        ss << "rel(area.a)"; add_filters(areaPattern, ss); ss << ";)->.s" << set_num++ << ";";
    }
    for(auto wayPattern : query_params["wayPatterns"]) {
        ss << "way(area.a)"; add_filters(wayPattern, ss); ss << "->.s" << set_num++ << ";";
    }
    ss << ".a out count;" << ".sa out geom;";
    for(int i=0; i<set_num; ++i)
        ss << ".s" << i << " out count;" << ".s" << i << " out body geom;";

    query = ss.str();

    return 0;
}


int osm_do_query(const std::string query, nlohmann::json & out_json) {
    try {
        Poco::Net::HTTPClientSession session("overpass-api.de", 80);
        session.setTimeout(Poco::Timespan(600, 0));

        std::string path("/api/interpreter");

        Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_POST, path, Poco::Net::HTTPMessage::HTTP_1_1);
        req.setContentLength(query.length());
        
        session.sendRequest(req) << query;

        Poco::Net::HTTPResponse res;
        std::istream& rs = session.receiveResponse(res);

        if(res.getStatus() != Poco::Net::HTTPResponse::HTTP_OK) {
            switch(res.getStatus()) {
                case 400:
                    std::cerr << "Query syntax error" << std::endl;
                    break;
                case 429:
                    std::cerr << "Too many requests to Overpass API" << std::endl;
                    break;
                case 504:
                    std::cerr << "Overpass API Gateway Timeout" << std::endl;
                    break;
                default: break;
            }
            std::cout << res.getStatus() << " " << res.getReason() << std::endl;
		    Poco::StreamCopier::copyStream(rs, std::cerr);

            return -1;
        }
        rs >> out_json;
    }
    catch(Poco::Exception &ex) {
        std::cerr << ex.displayText() << std::endl;
        return -1;
    }

    return 0;
}