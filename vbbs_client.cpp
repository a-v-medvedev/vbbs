#include <unistd.h>
#include <iostream>
#include <string>
#include "sockpp/tcp_connector.h"


int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    std::string host = "localhost";
    in_port_t port = 13345;
    
    sockpp::socket_initializer  sockInit;
    sockpp::tcp_connector       conn;

    if (!conn.connect(sockpp::inet_address(host, port))) {
        std::cerr << "Error connecting" << std::endl;
        return 1;
    }
    char local[1024];
    gethostname(local, 1024);
    std::string s = local;
    //std::string s = "A";
    while (true) {
        if (conn.write(s) != (int)s.length()) {
            std::cerr << "Error writing to the TCP stream: " << conn.last_error_str() << std::endl;
            break;
        }
        sleep(1000);
    }
    return 0;
}
