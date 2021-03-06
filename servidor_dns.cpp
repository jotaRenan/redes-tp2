#ifndef SERVIDOR
#define SERVIDOR

#include "utils.h"
#include "input_reader.hpp"
#include "linker.hpp"
#include "searcher.hpp"
#include "connection_handler.hpp"

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
#include <pthread.h>

#define MIN_ARGC 2

using namespace std;

int read_command(InputReader &reader, std::map<std::string, std::string> &dns_table,
                 std::list<std::tuple<int, struct sockaddr_storage>> &connections);

void usage(int argc, char **argv)
{
    printf("usage: %s <server port> [startup]\n", argv[1]);
    printf("example: %s 51511 \n", argv[0]);
    printf("example: %s 51511 in.txt\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC)
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
        perror("Error creating thread");
        return -1;
    }

    InputReader reader;
    if (argc == 3)
    {
        reader.read_file_to_buffer(argv[2]);
    }

    while (true)
    {
        int success = read_command(reader, dns_table, connections);
        if (success < 0)
        {
            usage(argc, argv);
        }
    }
    exit(EXIT_SUCCESS);
}

int read_command(InputReader &reader, std::map<std::string, std::string> &dns_table,
                 std::list<std::tuple<int, struct sockaddr_storage>> &connections)
{
    string command = reader.read();
    if (command == "add")
    {
        string hostname = reader.read();
        string ip = reader.read();
        dns_table[hostname] = ip;
    }
    else if (command == "search")
    {
        string hostname = reader.read();
        string ip = search(connections, dns_table, hostname);
        cout << (ip.empty() ? "-1" : ip) << endl;

    }
    else if (command == "link")
    {
        string ip = reader.read();
        string port = reader.read();
        if (link(connections, ip, port) < 0)
        {
            cout << "Invalid address." << endl;
            return 0;
        }
        cout << "Link created." << endl;
    }
    else
    {
        return -1;
    }
    return 0;
}

#endif