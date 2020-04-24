#pragma once

#include <unistd.h>

namespace sys
{
    static inline std::string myhostname()
    {
        char local[1024];
        gethostname(local, 1024);
        local[1024-1] = 0;
        return std::string(local);
    }

    static inline void defunct_node(const std::string &name) {
        std::stringstream ss;
        ss << "vbbs " << "defunct " << name;
        int r = system(ss.str().c_str());
        std::cout << ss.str() << " ret=" << r << std::endl;
    }

    static inline void add_node(const std::string &name) {
       std::stringstream ss;
       ss << "vbbs " << "add " << name;
       int r = system(ss.str().c_str());
       std::cout << ss.str() << " ret=" << r << std::endl;
    }

}

