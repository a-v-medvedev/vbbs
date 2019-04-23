#include <iostream>
#include <thread>
#include "sockpp/tcp_acceptor.h"
#include <vector>
#include <map>
#include <chrono>

#include <sstream>

static void str_split(const std::string& s, char delimiter, std::vector<std::string> &result)
{
   std::string token;
   std::istringstream token_stream(s);
   while (std::getline(token_stream, token, delimiter)) {
      result.push_back(token);
   }
}


// --------------------------------------------------------------------------
// The thread function. This is run in a separate thread for each socket.
// Ownership of the socket object is transferred to the thread, so when this
// function exits, the socket is automatically closed.

struct node {
    std::vector<sockpp::tcp_socket> socks;
    std::string name;
};

std::map<std::string, node> nodes;

void run_echo(int x)
{
    (void)x;
    char buf[512] = { 0, };

    while (true) {
        for (auto &n : nodes) {
            bool disconnected = false;
            for (auto it = n.second.socks.begin(); it != n.second.socks.end();) {
                int c = it->read(buf, 512);
                if (c > 0) {
                    buf[c] = 0;
                    if (n.second.name == "") {
                       n.second.name = buf;
                       std::cout << "vbbs " << "add " << buf << std::endl;
                    }
                    ++it;
                } else if (c == 0) {
                    std::cout << "vbbs " << "defunct " << n.second.name << std::endl;
                    disconnected = true;
                    ++it;
                    break;
                } else {
                    int err = it->last_error();
                    if (err != EAGAIN) {
                        std::cout << ">> c=" << c << " err=" << err << std::endl;
                    }
                    ++it;
                }
            }
            if (disconnected) {
                n.second.socks.resize(0);
                n.second.name = "";
            }
        }
        usleep(50000);
    }
/*
    while ((n = sock.read(buf, sizeof(buf))) > 0) {
        std::cout << "RECVD: " << buf << std::endl;
    }
        //sock.write_n(buf, n);

    std::cout << "Connection closed from " << sock.peer_address() << std::endl;
*/
}

// --------------------------------------------------------------------------
// The main thread runs the TCP port acceptor. Each time a connection is
// made, a new thread is spawned to handle it, leaving this main thread to
// immediately wait for the next connection.

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    in_port_t port = 13345;

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
        }
        else {
            // Create a thread and transfer the new stream to it.
            std::stringstream ss;
            ss << sock.peer_address();
            std::vector<std::string> s;
            str_split(ss.str(), ':', s);
            sock.read_timeout(std::chrono::microseconds(1000));
            nodes[s[0]].socks.push_back(std::move(sock));
        }
    }

    return 0;
}

