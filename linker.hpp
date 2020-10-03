#ifndef LINKER
#define LINKER

#include <list>
#include <sys/socket.h>
#include <tuple>

#include "utils.h"

using namespace std;
int link(
    list<tuple<int, struct sockaddr_storage>> &connections,
    string ip,
    string porta
);
int create_link_socket(string ip, string port, int &s, struct sockaddr_storage &storage);

#endif