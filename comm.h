#pragma once

#include "sockpp/tcp_acceptor.h"
#include "sockpp/tcp_connector.h"

namespace comm {
    static inline int read_str(sockpp::stream_socket &s, std::string &tag, std::string &str) {
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

    static inline std::shared_ptr<sockpp::tcp_acceptor> make_acceptor(int port) {
        auto acc = std::make_shared<sockpp::tcp_acceptor>(port);
        if (!(*acc)) {
            std::cerr << "vbbs_server: error creating an acceptor: "
                      << acc->last_error_str() << std::endl;
            throw 1;
        }
        return acc;
    }

    static inline sockpp::tcp_socket accept_connection(sockpp::tcp_acceptor &acc) {
        sockpp::inet_address peer;
        return std::move(acc.accept(&peer));
    }

    static inline bool check_if_socket_is_ok(sockpp::tcp_socket &sock, const sockpp::tcp_acceptor &acc) {
        if (!sock) {
            std::cerr << "vbbs_server: error accepting incoming connection: "
                      << acc.last_error_str() << std::endl;
            return false;
        }
        return true;
    }

    static inline std::string get_peer_name(sockpp::tcp_socket &sock) {
        std::stringstream ss;
        ss << sock.peer_address();
        std::vector<std::string> s;
        str_split(ss.str(), ':', s);
        return s[0];
    }

    static inline void set_sock_params(sockpp::tcp_socket &sock) {
        sock.read_timeout(std::chrono::microseconds(1000));
    }

    static inline std::shared_ptr<sockpp::tcp_connector> get_new_sock()
    {
        return std::make_shared<sockpp::tcp_connector>();
    }

    static inline bool connect(std::shared_ptr<sockpp::tcp_connector> &sock,
                               const std::string &host, in_port_t port) {
        bool connected = false;
        for (int i = 0; i < 5; i++) {
            std::cout << "vbbs_client: connecting to " << host << " " << port << std::endl;
            if (!sock->connect(sockpp::inet_address(host, port))) {
                continue;
            }
            connected = true;
            break;
        }
        if (!connected) {
            std::cerr << "vbbs_client: error connecting to " << host << " port="
                      << port << std::endl;
            return false;
        }
        return true;
    }

    static inline bool write_str(std::shared_ptr<sockpp::tcp_connector> &sock, const std::string &tag,
                   const std::string &str)
    {
        std::stringstream ss;
        ss << tag << ":" << str;
        std::string msg = ss.str() + "\0";
        char l = (char)(msg.length() & 0xff);
        if (sock->write_n(&l, 1) != 1)
            return false;
        if (sock->write_n(msg.c_str(), msg.length()) != (int)msg.length())
            return false;
        return true;
    }

    static inline bool write_str(std::shared_ptr<sockpp::tcp_connector> &sock, const std::string &tag,
                   int value)
    {
        std::stringstream ss;
        ss << value;
        return write_str(sock, tag, ss.str());
    }

    static inline std::string lasterror(std::shared_ptr<sockpp::tcp_connector> &sock)
    {
        return sock->last_error_str();
    }
}




