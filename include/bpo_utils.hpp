#ifndef BPO_UTILS_HPP
#define BPO_UTILS_HPP

#include <boost/program_options.hpp>

namespace BPOUtils {
    void conflicting_options(const boost::program_options::variables_map & vm,
                            const std::string & opt1, const std::string & opt2) {
        if(vm.count(opt1) && !vm[opt1].defaulted() && vm.count(opt2) && !vm[opt2].defaulted()) {
            throw std::logic_error(std::string("Conflicting options '") + opt1 + "' and '" + opt2 + "'.");
        }
    }
}

#endif // BPO_UTILS