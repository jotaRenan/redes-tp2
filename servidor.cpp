#ifndef SERVIDOR
#define SERVIDOR

#include "utils.h"
#include "forca.h"
#include <stdlib.h>
#include <list>
#include <tuple>
#include <map>
#include <queue>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <stdbool.h>
#include <string.h>
#include <pthread.h> //for threading
#include "input_reader.hpp"

#define BUFSZ 1024
#define MAX_PENDING_CONNECTIONS 10
#define MIN_ARGC 1
using namespace std;

struct thread_p
{
    char *port;
    std::map<std::string, std::string> *dns_table;
    std::list<std::tuple<int, struct sockaddr_storage>> *connections;
};

void *connection_handler(void *);
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

int link(
    list<tuple<int, struct sockaddr_storage>> &connections,
    string ip,
    string porta
);
int create_link_socket(string ip, string port, int &s, struct sockaddr_storage &storage);

void usage(int argc, char **argv)
{
    printf("usage: %s <server port> [startup]\n", argv[1]);
    printf("example: %s 51511 \n", argv[0]);
    printf("example: %s 51511 in.txt\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage(argc, argv);
    }
    std::map<std::string, std::string> dns_table = std::map<std::string, std::string>();
    std::list<std::tuple<int, struct sockaddr_storage>> connections;

    struct thread_p thread_params;
    thread_params.connections = &connections;
    thread_params.dns_table = &dns_table;
    thread_params.port = argv[1];

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, connection_handler, (void *)&thread_params) < 0)
    {
        perror("could not creeate thread");
        return -1;
    }

    InputReader reader;
    if (argc == 3) {
        reader.read_file_to_buffer(argv[2]);
    }    

    string command;
    while (!reader.buffer_is_empty())
    {
        command = reader.read();
        if (command == "add")
        {
            string hostname = reader.read();
            string ip = reader.read();
            dns_table[hostname] = ip;
        } else if (command == "search")
        {
            string hostname = reader.read();
            search(connections, dns_table, hostname);

        } else if (command == "link")
        {
            string ip = reader.read();
            string port = reader.read();
            if (link(connections, ip, port) < 0) {
                cout << "Invalid address." << endl;
                continue;
            }            
            cout << "Link created." << endl;
        } else
        {
            cout << "Failed at: " << command << endl;
            usage(argc, argv);
        }
    }
    while(true) {}
    exit(EXIT_SUCCESS);
}

void *connection_handler(void *params) // "SERVER"
{
    struct thread_p p = *(struct thread_p *)params;
    int socket_desc;
    struct sockaddr_storage server;
    memset(&server, 0, sizeof(server));
    if (server_sockaddr_init(IP_VERSION, p.port, &server) < 0) {
        logexit("init");
    }

    socket_desc = socket(server.ss_family, SOCK_DGRAM, 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
        logexit("socket thread");
    }
    printf("Socket created at port %s\n", p.port);

    struct sockaddr *servaddr = (struct sockaddr *)&server;
    if (bind(socket_desc, servaddr, sizeof(server)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    puts("bind done");

    struct sockaddr_storage client;
    memset(&client, 0, sizeof(client));
    socklen_t len = sizeof(client);
    char client_message[BUFSZ];

    puts("Waiting for datagram...");
    size_t read_size = 0;
    
    while ((read_size = recvfrom(socket_desc, (char *)client_message, BUFSZ,
                                 MSG_WAITALL, (struct sockaddr *)&client,
                                 &len)) > 0)
    {
        client_message[read_size] = '\0';
        string str_client_message(client_message);

        string hostname = str_client_message.substr(1);
        printf("Server received: %s\n", hostname.c_str());
        auto elemento = p.dns_table->find(hostname);
        bool found = elemento != p.dns_table->end();

        char response[BUFSZ];
        memset(response, 0, BUFSZ);
        response[0] = 2;

        if (found) {
            for (size_t i = 0; i < elemento->second.length(); i++) {
                response[i+1] = elemento->second.at(i);
            }
        } else {
            string result = search_neighbours(*p.connections, *p.dns_table, hostname);
            if (result.empty()) {
                response[1] = -1;
            } else {
                for (size_t i = 0; i < result.length(); i++) {
                    response[i+1] = result.at(i);
                }
            }
        }

        int bytes_sent = sendto(
            socket_desc, 
            (const char *)response, 
            strlen(response), 
            MSG_CONFIRM, 
            (struct sockaddr*)&client,
            len
        );
        if (bytes_sent < 0) {
            logexit("sendto");
        }
        printf("Server sent \"%s\"\n", response);
        memset(client_message, 0, BUFSZ);
        memset(&client, 0, sizeof(client));
        len = sizeof(client);
    }

    if (read_size == (size_t)0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if (read_size == (size_t)-1)
    {
        perror("recv failed");
    }

    return 0;
}

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
        cout << "Buscando externamente (" << sockfd << ")\n";
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
    }
    return 0;
}

#endif