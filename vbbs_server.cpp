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

struct node {
    std::vector<sockpp::tcp_socket> socks;
    std::string name;
    std::deque<int> prod;
};

// NOTE: global variables can be accessed only via lock!
namespace global {
    std::mutex lock;
    std::map<std::string, node> nodes;
}

namespace sys {
void defunct_node(const std::string &name) {
    std::stringstream ss;
    ss << "vbbs " << "defunct " << name;
    int r = system(ss.str().c_str());
    std::cout << ss.str() << " ret=" << r << std::endl;
}

void add_node(const std::string &name) {
   std::stringstream ss;
   ss << "vbbs " << "add " << name;
   int r = system(ss.str().c_str());
   std::cout << ss.str() << " ret=" << r << std::endl;
}
}

namespace comm {
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

    std::shared_ptr<sockpp::tcp_acceptor> make_acceptor(int port) {
        auto acc = std::make_shared<sockpp::tcp_acceptor>(port);
        if (!(*acc)) {
            std::cerr << "vbbs_server: error creating an acceptor: " 
                      << acc->last_error_str() << std::endl;
            throw 1;
        }
        return acc;
    }

    sockpp::tcp_socket accept_connection(sockpp::tcp_acceptor &acc) {
        sockpp::inet_address peer;
        return std::move(acc.accept(&peer));
    }

    bool check_if_socket_is_ok(sockpp::tcp_socket &sock, const sockpp::tcp_acceptor &acc) {
        if (!sock) {
            std::cerr << "vbbs_server: error accepting incoming connection: "
                      << acc.last_error_str() << std::endl;
            return false;
        }
        return true;
    }

    std::string get_peer_name(sockpp::tcp_socket &sock) {
        std::stringstream ss;
        ss << sock.peer_address();
        std::vector<std::string> s;
        str_split(ss.str(), ':', s);
        return s[0];
    }

    void set_sock_params(sockpp::tcp_socket &sock) {
        sock.read_timeout(std::chrono::microseconds(1000));
    }    
}

static inline int get_num_socks()
{
    std::lock_guard<std::mutex> guard(global::lock);
    int N = 0;
    for (auto &n : global::nodes) {
        auto &nd = n.second;
        N += nd.socks.size();
    }
    return N;
}

static inline void add_socket_to_node(const std::string &str, sockpp::tcp_socket &&sock) 
{
    std::lock_guard<std::mutex> guard(global::lock);
    global::nodes[str].socks.push_back(std::move(sock));
}

void handle_message(node &nd, const std::string &tag, const std::string &str)
{
    if (tag == "name" && nd.name == "") {
       nd.name = str;
       sys::add_node(nd.name);
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
                std::cout << "vbbs_server: " << nd.name << " " << v << " " << prod << std::endl;
                // TODO defunct this node??
            }
        }
    }
}

void server_loop(int x)
{
    (void)x;
    while (true) {
        { 
            std::lock_guard<std::mutex> guard(global::lock);
            for (auto &n : global::nodes) {
                bool disconnected = false;
                auto &nd = n.second;
                for (auto it = nd.socks.begin(); it != nd.socks.end(); ++it) {
                    std::string tag, str;                
                    int c = comm::read_str(*it, tag, str);
                    if (c > 0) {
                        handle_message(nd, tag, str);
                    } else if (c == 0) {
                        int err = it->last_error();
                        std::cerr << "vbbs_server: disconnected: node=" << nd.name 
                                  << " c=" << c << " err=" << err << std::endl;
                        if (nd.name != "") 
                            sys::defunct_node(nd.name);
                        disconnected = true;
                        break;
                    } else {
                        int err = it->last_error();
                        if (err != EAGAIN) {
                            std::cerr << "vbbs_server: connection error: node=" << nd.name 
                                      << " c=" << c << " err=" << err << std::endl;
                            if (nd.name != "") 
                                sys::defunct_node(nd.name);
                            disconnected = true;
                        }
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
        std::cerr << "vbbs_server: environment variable " << varname 
                  << " is not set, applying defaults" << std::endl;
    }

    try {    
        auto acceptor = comm::make_acceptor(port);
        std::cout << "vbbs_server: awaiting connections on port " << port 
                  << "..." << std::endl;
        std::thread thr(server_loop, 0);
        thr.detach();
        while (true) {
            sockpp::tcp_socket sock = comm::accept_connection(*acceptor);
            if (!comm::check_if_socket_is_ok(sock, *acceptor)) {
                usleep(1000);
                if (get_num_socks() > 900) {
#ifdef WITH_DEBUG        
                    std::cout << "vbbs_server: too many sockets, waiting for a second..." 
                              << std::endl;
#endif
                    sleep(1);
                    continue;
                }
            }
            auto peer_name = comm::get_peer_name(sock);
            comm::set_sock_params(sock);
            add_socket_to_node(peer_name, std::move(sock));
#ifdef WITH_DEBUG            
            std::cout << "vbbs_server: socket for " << peer_name << " created and added" 
                      << std::endl;
#endif            
        }
    }
    catch (int x) {    
        std::cerr << "EXCEPTION: x=" << x << std::endl;
    }
    catch (const std::exception &ex) {
        std::cerr << "EXCEPTION: " << ex.what() << std::endl;
    }
    catch (...) {
        std::cerr << "EXCEPTION!" << std::endl;
    }
    return 0;
}

