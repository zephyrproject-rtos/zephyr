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
#include <zephyr/net/socket.h>
#include <unistd.h>
#include <signal.h>

#include "headers/config.h"
#include "headers/index_html_gz.h"
#include "headers/server_functions.h"
#include <zephyr/net/http/parser.h>

static char url_buffer[MAX_URL_LENGTH];
static char buffer[BUFFER_SIZE] = {0};
static char http_response[BUFFER_SIZE];
static struct pollfd fds[MAX_CLIENTS+1];

static unsigned char settings_frame[9] = {0x00, 0x00, 0x00, /* Length */
	0x04, /* Type: 0x04 - setting frames for config or acknowledgment */
	0x00, /* Flags: 0x00 - unused flags */
	0x00, 0x00, 0x00,
	0x00}; /* Reserved, Stream Identifier: 0x00 - overall connection */

static unsigned char settings_ack[9] = {0x00, 0x00, 0x00, /* Length */
	0x04, /* Type: 0x04 - setting frames for config or acknowledgment */
	0x01, /* Flags: 0x01 - ACK */
	0x00, 0x00, 0x00, 0x00}; /* Reserved, Stream Identifier */

/* Signal handler function */
void handle_shutdown(int signum)
{
	printf("Shutting down...\n");
	exit(EXIT_SUCCESS);
}

int main(void)
{
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	int server_fd = create_server_socket(&address);

	printf("Press 'Ctrl+C' to quit\n");
	printf("Waiting for incoming connections on port 8000...\n");

	/* Register the signal handler */
	signal(SIGINT, handle_shutdown);

	memset(fds, 0, sizeof(fds));
	fds[0].fd = server_fd;
	fds[0].events = POLLIN;

	while (1) {
		int ret = poll(fds, MAX_CLIENTS, -1);

		if (ret < 0) {
			perror("poll failed");
			exit(EXIT_FAILURE);
		}

		if (fds[MAX_CLIENTS].revents & POLLIN) {
			char ch;

			read(STDIN_FILENO, &ch, 1);
			if (ch == 'q') {
				printf("Shutting down...\n");
				break;
			}
		}

		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (!fds[i].revents & POLLIN)
				continue;
			if (i == 0) {
				int new_socket = accept_new_client(
				    server_fd, &address, &addrlen);

				for (int j = 1; j < MAX_CLIENTS; j++) {
					if (fds[j].fd == 0) {
						fds[j].fd = new_socket;
						fds[j].events = POLLIN;
						break;
					}
				}
			}
	}

	printf("Shutting down...\n");
	return 0;
}

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

		if (out_len < 0) {
			return out_len;
		}
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

	if (SOCKET_FAMILY == AF_INET) {
		struct sockaddr_in *addr_in = (struct sockaddr_in *)address;

		addr_in->sin_family = AF_INET;
		addr_in->sin_addr.s_addr = INADDR_ANY;
		addr_in->sin_port = htons(PORT);

		if (bind(server_fd, (struct sockaddr *)addr_in,
			 sizeof(*addr_in)) < 0) {
			perror("bind failed");
			exit(EXIT_FAILURE);
		}
	} else if (SOCKET_FAMILY == AF_INET6) {
		struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)address;

		addr_in6->sin6_family = AF_INET6;
		addr_in6->sin6_addr = in6addr_any;
		addr_in6->sin6_port = htons(PORT);

		if (bind(server_fd, (struct sockaddr *)addr_in6,
			 sizeof(*addr_in6)) < 0) {
			perror("bind failed");
			exit(EXIT_FAILURE);
		}
	} else {
		fprintf(stderr, "Invalid socket family\n");
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

	new_socket =
	    accept(server_fd, (struct sockaddr *)address, (socklen_t *)addrlen);
	if (new_socket < 0) {
		perror("accept failed");
		return -1;
	}

	return new_socket;
}

void handle_http2_request(int i, const char *buffer)
{
	ssize_t n = write(fds[i].fd, settings_frame, sizeof(settings_frame));

	if (n < 0) {
		perror("ERROR writing to socket");
		handleError(i);
	} else {
		n = write(fds[i].fd, settings_ack, 9);
	}
}

void handle_http1_request(int i, const char *buffer)
{
	struct http_parser_settings parserSettings;
	struct http_parser parser;

	http_parser_init(&parser, HTTP_REQUEST);
	http_parser_settings_init(&parserSettings);
	parserSettings.on_url = (http_data_cb)on_url;

	size_t bytesRead = strlen(buffer);

	http_parser_execute(&parser, &parserSettings, buffer, bytesRead);

	if (strcmp(url_buffer, "/") == 0) {
		sprintf(http_response,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Encoding: gzip\r\n"
			"Content-Length: %u\r\n\r\n",
			src_index_html_gz_len);
		if (sendall(fds[i].fd, http_response, strlen(http_response)) <
		    0) {
			handleError(i);
		} else {
			if (sendall(fds[i].fd, src_index_html_gz,
				    src_index_html_gz_len) < 0) {
				perror("sendall failed");
				handleError(i);
			}
		}
	} else {
		const char *not_found_response = "HTTP/1.1 404 Not Found\r\n"
						 "Content-Length: 9\r\n\r\n"
						 "Not Found";
		if (sendall(fds[i].fd, not_found_response,
			    strlen(not_found_response)) < 0) {
			handleError(i);
		}
	}
	close(fds[i].fd);
	fds[i].fd = 0;
}
