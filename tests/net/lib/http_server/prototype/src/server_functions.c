/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netinet/in.h>
#include <zephyr/posix/poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headers/server_functions.h"

static char url_buffer[MAX_URL_LENGTH];

static struct pollfd fds[MAX_CLIENTS + 1];

const char *on_url(struct http_parser *parser, const char *at, size_t length)
{
	strncpy(url_buffer, at, length);
	url_buffer[length] = '\0';
	printf("Requested URL: %s\n", url_buffer);
	return url_buffer;
}

ssize_t sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = send(sock, buf, len, 0);

		if (out_len < 0)
			return out_len;

		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

void handleError(int clientIndex)
{
	close(fds[clientIndex].fd);
	fds[clientIndex].fd = 0;
	fds[clientIndex].events = 0;
	fds[clientIndex].revents = 0;
}

int create_server_socket(struct sockaddr_in *address)
{
	int server_fd;

	server_fd = socket(SOCKET_FAMILY, SOCK_STREAM, 0);

	if (server_fd < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	address->sin_family = SOCKET_FAMILY;
	address->sin_addr.s_addr = INADDR_ANY;
	address->sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr *)address, sizeof(*address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, MAX_CLIENTS) < 0) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}

	return server_fd;
}

int accept_new_client(int server_fd, struct sockaddr_in *address, int *addrlen)
{
	int new_socket;

	new_socket = accept(server_fd, (struct sockaddr *)address, (socklen_t *)addrlen);

	if (new_socket < 0) {
		perror("accept failed");
		return -1;
	}

	return new_socket;
}
