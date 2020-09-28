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
                    cout << "Buscando externamente\n";

                    int sockfd = std::get<0>(con_tuple);
                    struct sockaddr_storage *storage = std::get<1>(con_tuple);
                    
                    int sent = sendto(
                        sockfd,
                        hostname.c_str(),
                        hostname.length(),
                        MSG_CONFIRM,
                        (const struct sockaddr *)storage,
                        sizeof(*storage)
                    );

                    if (sent < 0 ) logexit("sendto");

                    char buffer[BUFSZ];
                    socklen_t len = sizeof(*storage);
                    int n;
                    n = recvfrom(
                        sockfd,
                        (char *)buffer,
                        BUFSZ,
                        MSG_WAITALL,
                        (struct sockaddr *)&storage,
                        &len
                    );
                    buffer[n] = '\0';
                    printf("Received : %s\n", buffer);

                    // TODO: verificar resposta.
                    // Se -1, continuar
                    // Se IP valido, adicionar entrada na tabela DNS e break
                }
            }
        }
        else if (command == "link")
        {
            string ip, porta;
            cin >> ip >> porta;

            struct sockaddr_storage storage;
            if (server_sockaddr_init(IP_VERSION, porta.c_str(), &storage) != 0)
            {
                usage(argc, argv);
            }

            int s = socket(storage.ss_family, SOCK_DGRAM, 0);
            if (s < 0)
            {
                logexit("link socket creation");
            }
            auto t_tuple = std::make_tuple(s, &storage);
            connections.push_back(t_tuple);
            cout << "Link created." << endl;
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

void *connection_handler(void *params) // "SERVER"
{
    struct thread_p p = *(struct thread_p *)params;
    int socket_desc;
    struct sockaddr_storage server;
    memset(&server, 0, sizeof(server));
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
        printf("Server received: %s\n", client_message);
        auto elemento = p.dns_table->find(client_message);
        cout << (elemento == p.dns_table->end() ?  "-1" : elemento->second);

        // TODO: implementar leitura da mensagem segundo estrutura do enunciado
        // TODO: chamar metodo para verificar se esta tabela de DNS possui entrada
        const char* response = "oi!";
        int bytes_sent;
        bytes_sent = sendto(
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
