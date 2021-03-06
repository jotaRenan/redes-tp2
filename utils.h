#pragma once

#include <arpa/inet.h>
#include <inttypes.h>

#define IP_VERSION "v6"
#define V6_LOOP "::1"
#define V4_LOOP "127.0.0.1"
#define BUFSZ 1024

void logexit(const char *msg);
int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);