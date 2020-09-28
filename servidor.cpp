#include "utils.h"
#include "forca.h"
#include <stdlib.h>
#include <list>
#include <tuple>
#include <map>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h> //for threading
#define BUFSZ 1024
#define MAX_PENDING_CONNECTIONS 10
#define MIN_ARGC 1
using namespace std;

struct thread_p
{
    char *port;
    std::map<std::string, std::string> *dns_table;
    std::list<std::tuple<int, struct sockaddr_storage *>> *connections;
};

void *connection_handler(void *);

void add_entry(std::map<std::string, std::string> table, std::string hostname, std::string ip)
{
    table.insert(std::pair<std::string, std::string>(hostname, ip));
}

void search_ip(std::string hostname, std::map<std::string, std::string> table, std::string &ip)
{
    auto it = table.find(hostname);
    if (it != table.end())
    {
        ip = it->second;
    }
    // if (search_ip_other_servers(hostname, ip))
    // {
    //     for (auto server : servers)
    //     {
    //         server.search_ip(hostname);
    //     }
    // }
}

void usage(int argc, char **argv)
{
    printf("usage: %s <server port> [startup]\n", argv[0]);
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
    std::list<std::tuple<int, struct sockaddr_storage *>> connections;

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

    string command;
    while (true)
    {

        cin >> command;

        if (command == "add")
        {
            // todo add
            string hostname, ip;
            cin >> hostname >> ip;
            dns_table.insert(std::pair<std::string, std::string>(hostname, ip));
        }
        else if (command == "search")
        {
            string hostname;
            cin >> hostname;
            auto it = dns_table.find(hostname);
            if (it != dns_table.end())
            {
                string ip = it->second;
                cout << ip;
            }
            else
            {
                for (auto const &con_tuple : connections)
                {
                    char buffer[BUFSZ];
                    //enviar requisicao pra cada socket perguntando ip
                    // se achar, add na dns_table
                    int sockfd = std::get<0>(con_tuple);
                    struct sockaddr_storage *storage = std::get<1>(con_tuple);
                    int n;
                    socklen_t len;
                    sendto(
                        sockfd,
                        hostname.c_str(),
                        hostname.length(),
                        MSG_CONFIRM,
                        (const struct sockaddr *)&storage,
                        sizeof(storage));
                    printf("Hello message sent.\n");

                    n = recvfrom(
                        sockfd,
                        (char *)buffer,
                        BUFSZ,
                        MSG_WAITALL,
                        (struct sockaddr *)&storage,
                        &len
                    );
                    buffer[n] = '\0';
                    printf("Server : %s\n", buffer);

                    // close(sockfd);
                }
            }
        }
        else if (command == "link")
        {
            // todo link
            string ip, porta;
            cin >> ip >> porta;

            struct sockaddr_storage storage;
            if (server_sockaddr_init(IP_VERSION, porta.c_str(), &storage) != 0)
            {
                usage(argc, argv);
            }

            int s = socket(storage.ss_family, SOCK_DGRAM, 0);
            if (s == -1)
            {
                logexit("socket");
            }
            int enable = 1;
            if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0)
            {
                logexit("setsockopt");
            }

            struct sockaddr *addr = (struct sockaddr *)(&storage);
            // bind
            cout << addr->sa_data << " " << addr->sa_family << " ";
            if (bind(s, addr, sizeof(storage)) != 0)
            {
                logexit("bind");
            }

            auto t_tuple = std::make_tuple(s, &storage);
            connections.push_back(t_tuple);
            // accept
            char addrstr[BUFSZ];
            addrtostr(addr, addrstr, BUFSZ);
            printf("Listening to %s, waiting for connection..\n", addrstr);
        }
        else
        {
            if (argc < MIN_ARGC)
            {
                usage(argc, argv);
            }
        }
    }
    exit(EXIT_SUCCESS);
}

void *connection_handler(void *params)
{
    struct thread_p p = *(struct thread_p *)params;
    int socket_desc, client_sock, c;
    char buffer[BUFSZ];
    struct sockaddr_storage server, client;
    if (addrparse("127.0.0.1", p.port, &server) != 0)
    {
        logexit("addrparse thread");
    }

    socket_desc = socket(server.ss_family, SOCK_DGRAM, 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
        logexit("socket thread");
    }
    puts("Socket created");
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));
    // Bind the socket with the server address
    struct sockaddr *servaddr = (struct sockaddr *)(&server);
    if (bind(socket_desc, servaddr, sizeof(server)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    puts("bind done");

    listen(socket_desc, 3);
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
    puts("Connection accepted");

    //Get the socket descriptor
    size_t read_size;
    socklen_t len;
    char client_message[2000];

    //Receive a message from client
    while ((read_size = recvfrom(socket_desc, (char *)buffer, BUFSZ,
                                 MSG_WAITALL, (struct sockaddr *)&client,
                                 &len)) > 0)
    {
        //end of string marker
        client_message[read_size] = '\0';
        printf("Server receive: %s\n", client_message);

        //Send the message back to client
        write(client_sock, client_message, strlen(client_message));

        //clear the message buffer
        memset(client_message, 0, 2000);
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

// todo: avaliar se precisa criar N threads
// criar estrutura das mensagens
