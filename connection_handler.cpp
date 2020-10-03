#include "connection_handler.hpp"
#include "searcher.hpp"
#include "utils.h"

#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>

using namespace std;

void *connection_handler(void *params) // "SERVER"
{
    struct thread_p p = *(struct thread_p *)params;
    int socket_desc = create_handler_socket(p.port);
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
        auto elemento = p.dns_table->find(hostname);
        bool found = elemento != p.dns_table->end();

        char response[BUFSZ];
        memset(response, 0, BUFSZ);
        response[0] = 2;

        if (found)
        {
            for (size_t i = 0; i < elemento->second.length(); i++)
            {
                response[i + 1] = elemento->second.at(i);
            }
        }
        else
        {
            string result = search_neighbours(*p.connections, hostname);
            if (result == "-1")
            {
                response[1] = -1;
            }
            else
            {
                for (size_t i = 0; i < result.length(); i++)
                {
                    response[i + 1] = result.at(i);
                }
            }
        }

        int bytes_sent = sendto(socket_desc, (const char *)response, strlen(response),
                                MSG_CONFIRM, (struct sockaddr *)&client, len);
        if (bytes_sent < 0)
        {
            logexit("sendto");
        }
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

int create_handler_socket(char *port)
{
    int socket_desc;
    struct sockaddr_storage server;
    memset(&server, 0, sizeof(server));
    if (server_sockaddr_init(IP_VERSION, port, &server) < 0)
    {
        logexit("init");
    }

    socket_desc = socket(server.ss_family, SOCK_DGRAM, 0);
    if (socket_desc == -1)
    {
        logexit("handler socket");
    }
    printf("Socket created at port %s\n", port);

    struct sockaddr *servaddr = (struct sockaddr *)&server;
    if (bind(socket_desc, servaddr, sizeof(server)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    return socket_desc;
}
