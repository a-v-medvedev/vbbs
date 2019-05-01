#include <iostream>
#include <thread>
#include "sockpp/tcp_acceptor.h"
#include <vector>
#include <map>
#include <chrono>
#include <deque>
#include <thread>
#include <mutex>

#include "utils.h"

std::mutex lock;

struct node {
    std::vector<sockpp::tcp_socket> socks;
    std::string name;
    std::deque<int> prod;
};

std::map<std::string, node> nodes;

void defunct(const std::string &name) {
    std::stringstream ss;
    ss << "vbbs " << "defunct " << name;
    int r =system(ss.str().c_str());
    std::cout << ss.str() << " " << r << std::endl;
}

void add(const std::string &name) {
   std::stringstream ss;
   ss << "vbbs " << "add " << name;
   int r =system(ss.str().c_str());
   std::cout << ss.str() << " " << r << std::endl;
}

int read_str(sockpp::stream_socket &s, std::string &tag, std::string &str) {
    int c1, c2; 
    int len = 0;
    char buf[512];
    c1 = s.read(buf, 1);
    if (c1 == 1) {
        len = (int)buf[0];
        c2 = s.read(buf, len);
        if (c2 == len) {
            buf[len] = 0;
            std::string msg(buf);
            std::vector<std::string> kv;
            str_split(msg, ':', kv);
            if (kv.size() != 2) {
                return 0;
            }
            tag = kv[0];
            str = kv[1];
            return 1;
        } else {
            return c2;
        }
    } else {
        return c1;
    }
}

int get_num_socks()
{
    std::lock_guard<std::mutex> guard(lock);
    int N = 0;
    for (auto &n : nodes) {
        auto &nd = n.second;
        N += nd.socks.size();
    }
    return N;
}

void add_socket_to_node(const std::string &str, sockpp::tcp_socket &&sock) {
    std::lock_guard<std::mutex> guard(lock);
    nodes[str].socks.push_back(std::move(sock));
}


void run_echo(int x)
{
    (void)x;
    while (true) {
        { 
            std::lock_guard<std::mutex> guard(lock);
            for (auto &n : nodes) {
                bool disconnected = false;
                auto &nd = n.second;
                for (auto it = nd.socks.begin(); it != nd.socks.end();) {
                    std::string tag, str;                
                    int c = read_str(*it, tag, str);
                    if (c > 0) {
                        if (tag == "name" && nd.name == "") {
                           nd.name = str;
                           add(nd.name);
                        }
                        if (tag == "prod") {
                            int prod = 0;
                            try {
                                prod = std::stoi(str);
                            }
                            catch(std::invalid_argument &) {}
                            catch(std::out_of_range &) {}
                            nd.prod.push_back(prod);
                            if (nd.prod.size() == 101) {
                                int v = nd.prod.front();
                                nd.prod.pop_front();
                                if (abs(v - prod) > 5 && v && prod) {
                                    std::cout << "VBBS: " << nd.name << " " << v << " " << prod << std::endl;
                                }
                           }
                        }
                        ++it;
                    } else if (c == 0) {
                        if (nd.name != "") 
                            defunct(nd.name);
                        disconnected = true;
                        ++it;
                        break;
                    } else {
                        int err = it->last_error();
                        if (err != EAGAIN) {
                            std::cerr << "VBBS: connection error: c=" << c << " err=" << err << std::endl;
                            if (nd.name != "") 
                                defunct(nd.name);
                            disconnected = true;
                        }
                        ++it;
                        break;
                    }
                }
                if (disconnected) {
                    for (auto &s : nd.socks)
                        s.close();
                    nd.socks.resize(0);
                    nd.name = "";
                }
            }
        }
        usleep(5000);
    }
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    std::string hostfile, host, sem;
    in_port_t port = 0;
    const std::string varname = "VBBS_PARAMS";
    if (!check_environment<in_port_t>(varname, hostfile, host, port, sem,
                                               "hostfile", "master", 13345, "vbbs_sem")) {
        std::cerr << "VBBS: environment variable " << varname << " is not set, applying defaults" << std::endl;
    }

    sockpp::socket_initializer  sockInit;
    sockpp::tcp_acceptor        acc(port);

    if (!acc) {
        std::cerr << "Error creating the acceptor: " << acc.last_error_str() << std::endl;
        return 1;
    }
    std::cout << "Awaiting connections on port " << port << "..." << std::endl;
    
    std::thread thr(run_echo, 0);
    thr.detach();

    while (true) {
        sockpp::inet_address peer;

        // Accept a new client connection
        sockpp::tcp_socket sock = acc.accept(&peer);
        //std::cout << "Received a connection request from " << peer << std::endl;

        if (!sock) {
            std::cerr << "Error accepting incoming connection: " 
                << acc.last_error_str() << std::endl;
            usleep(1000);
            if (get_num_socks() > 900) {
                sleep(1);
            }
        }
        else {
            std::stringstream ss;
            ss << sock.peer_address();
            std::vector<std::string> s;
            str_split(ss.str(), ':', s);
            sock.read_timeout(std::chrono::microseconds(1000));
            std::lock_guard<std::mutex> guard(lock);
            // TODO: add_socket_to_node(s[0], std::move(sock));
            nodes[s[0]].socks.push_back(std::move(sock));
        }
    }

    return 0;
}

