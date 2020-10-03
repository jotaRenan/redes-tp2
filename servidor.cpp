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
#include "linker.hpp"
#include "searcher.hpp"

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
        logexit("handler socket");
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

    puts("Waiting for datagram...");
    
    char client_message[BUFSZ];
    socklen_t len = sizeof(client);
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

        int bytes_sent = sendto(socket_desc, (const char *)response, strlen(response),
                                    MSG_CONFIRM, (struct sockaddr*)&client, len);
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

#endif