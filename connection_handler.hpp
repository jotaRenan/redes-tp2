#ifndef CONNECTION_HANDLER
#define CONNECTION_HANDLER
# include <map>
# include <list>
# include <tuple>

void *connection_handler(void *);
int create_handler_socket(char *port);


struct thread_p
{
    char *port;
    std::map<std::string, std::string> *dns_table;
    std::list<std::tuple<int, struct sockaddr_storage>> *connections;
};

#endif