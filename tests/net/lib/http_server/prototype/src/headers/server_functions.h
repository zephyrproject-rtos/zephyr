#ifndef SERVER_FUNCTIONS_H
#define SERVER_FUNCTIONS_H

#include "config.h"
#include <zephyr/net/http/parser.h>


const char *on_url(struct http_parser *parser, const char *at, size_t length);
ssize_t sendall(int sock, const void *buf, size_t len);
void handleError(int clientIndex);
int create_server_socket(struct sockaddr_in *address);
int accept_new_client(int server_fd, struct sockaddr_in *address, int *addrlen);

#endif
