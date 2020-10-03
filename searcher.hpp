#ifndef SEARCHER
#define SEARCHER

#include <list>
#include <sys/socket.h>
#include <tuple>
#include <map>

using namespace std;

void search(
    list<tuple<int, struct sockaddr_storage>> &connections,
    map<string, string> &dns_table,
    string hostname
);

string search_neighbours(
    list<tuple<int, struct sockaddr_storage>> &connections,
    map<string, string> &dns_table,
    string hostname
);
#endif