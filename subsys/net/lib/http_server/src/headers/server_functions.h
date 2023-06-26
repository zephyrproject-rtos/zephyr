/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SERVER_FUNCTIONS_H
#define SERVER_FUNCTIONS_H

#include <zephyr/net/http/parser.h>
#include "config.h"

const char *on_url(struct http_parser *parser, const char *at, size_t length);
ssize_t sendall(int sock, const void *buf, size_t len);
void handleError(int clientIndex);
int create_server_socket(struct sockaddr_in *address);
int accept_new_client(int server_fd, struct sockaddr_in *address, int *addrlen);
void handle_client_data(int i, char *buffer);
void handle_incoming_clients(int server_fd, struct sockaddr_in *address, int *addrlen);
void handle_http1_request(int i, const char *buffer);
void handle_http2_request(int i, const char *buffer);

#endif
