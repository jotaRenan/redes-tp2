#pragma once

#include <arpa/inet.h>
#include <inttypes.h>

#define IP_VERSION "v6"
#define V6_LOOP "::1"
#define V4_LOOP "127.0.0.1"

void logexit(const char *msg);
int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);
const char* get_loopback_address();