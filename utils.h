#pragma once

#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>


namespace utils {

namespace str {

static std::vector<std::string> split(const std::string& s, char delimiter)
{
    std::vector<std::string> result;
    std::string token;
    std::istringstream token_stream(s);
    while (std::getline(token_stream, token, delimiter)) {
        result.push_back(token);
    }
    return result;
}

static inline bool contains(const std::string &s, char c) {
    return s.find(c) < s.length();
}

std::string getenv(const std::string &var, const std::string &error) {
    auto ptr = std::getenv(var.c_str());
    if (ptr == nullptr) {
        throw std::runtime_error(error);
    }
    return std::string(ptr);
}

static std::string to_string_with_leading_zeroes(int n, size_t length) {
    auto s = std::to_string(n);
    return std::string(length - std::min(length, s.length()), '0') + s;
}

}

}

static void str_split(const std::string& s, char delimiter, std::vector<std::string> &result)
{
   std::string token;
   std::istringstream token_stream(s);
   while (std::getline(token_stream, token, delimiter)) {
      result.push_back(token);
   }
}

template <typename UINT>
bool check_environment(const std::string &varname, std::string &hostfile, std::string &host, UINT &port, 
                       std::string &sem, const std::string &def_hostfile, 
                       const std::string &def_host, UINT def_port, const std::string &def_sem) {
    bool r = true;
    char *value = std::getenv(varname.c_str());
    if (value == NULL) {
        r = false;
    }
    if (r) {
        std::vector<std::string> s;
        str_split(value, ':', s);
        if (s.size() == 0) {
            return false;
        } else if (s.size() == 1) {
            hostfile = s[0];
        } else if (s.size() == 2) {
            hostfile = s[0];
            host = s[1];
        } else if (s.size() == 3) {
            hostfile = s[0];
            host = s[1];
            port = (UINT)std::stoi(s[2]);
        } else if (s.size() == 4) {
            hostfile = s[0];
            host = s[1];
            port = (UINT)std::stoi(s[2]);
            sem = s[3];
        } else {
            r = false;
        }
    }
    if (hostfile == "")
        hostfile = def_hostfile;
    if (host == "")
        host = def_host;
    if (port == 0)
        port = def_port;
    if (sem == "")
        sem = def_sem;
#ifdef WITH_DEBUG    
    std::cout << ">> Environment: " << hostfile << " " << host << " " << port << " " << sem << " " << std::endl;
#endif    
    return r;
}
