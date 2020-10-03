#include "searcher.hpp"
#include "utils.h"
#include <iostream>
#include <string.h>

void search(
    list<tuple<int, struct sockaddr_storage>> &connections,
    map<string, string> &dns_table,
    string hostname
) {
    auto it = dns_table.find(hostname);
    if (it != dns_table.end())
    {
        string ip = it->second;
        cout << ip;
    }
    else
    {
        search_neighbours(connections, dns_table, hostname);
    }
}

string search_neighbours(
    list<tuple<int, struct sockaddr_storage>> &connections,
    map<string, string> &dns_table,
    string hostname
) {
    for (auto const &con_tuple : connections)
    {
        char message[BUFSZ];
        memset(message, 0, BUFSZ);
        message[0] = -1;
        for (size_t i = 0; i < hostname.length(); i++) {
            message[i+1] = hostname.at(i);
        }

        int sockfd = std::get<0>(con_tuple);
        struct sockaddr_storage storage = std::get<1>(con_tuple);
        
        int sent = sendto(sockfd, (const char *)message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&storage, sizeof(storage));

        if (sent < 0 ) logexit("sendto");

        char buffer[BUFSZ];
        socklen_t len = sizeof(storage);
        int n = recvfrom(sockfd, (char *)buffer, BUFSZ, MSG_WAITALL, (struct sockaddr *)&storage, &len);
        buffer[n] = '\0';
        printf("Received : %s\n", buffer);

        if (buffer[0] == 2 && buffer[1] != -1) {
            string ip = string(buffer).substr(1);
            cout << "Found! IP address:" << ip << endl;
            dns_table[hostname] = ip;
            return ip;
        }
    }
    cout << "Not found" << endl;
    return "";
}
