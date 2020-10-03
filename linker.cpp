#include "linker.hpp"

int link(
    list<tuple<int, struct sockaddr_storage>> &connections,
    string ip,
    string port
) {
    int s;
    struct sockaddr_storage storage;
    if (create_link_socket(ip, port, s, storage) < 0) {
        return -1;    
    }
    auto t_tuple = std::make_tuple(s, storage);
    connections.push_back(t_tuple);
    return 0;
}

int create_link_socket(string ip, string port, int &s, struct sockaddr_storage &storage) {
    if (addrparse(ip.c_str(), port.c_str(), &storage) < 0)
    {
        return -1;
    }

    s = socket(storage.ss_family, SOCK_DGRAM, 0);
    if (s < 0)
    {
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error");
        return -1;
    }
    return 0;
}
