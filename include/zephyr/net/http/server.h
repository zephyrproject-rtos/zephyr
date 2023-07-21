/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_SERVER_H_
#define ZEPHYR_INCLUDE_NET_HTTP_SERVER_H_

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)

#define CONFIG_NET_HTTP2_MAX_CLIENTS 10
#include <poll.h>

#else

#include <zephyr/posix/poll.h>

#endif

#define MAX_CLIENTS CONFIG_NET_HTTP2_MAX_CLIENTS

struct http2_server_config {
	int port;
	int address_family;
};

enum http2_server_state {
	AWAITING_PREFACE,
	READING_SETTINGS,
	STREAMING,
	CLOSING
};

struct http2_client_ctx {
	int client_fd;
	enum http2_server_state state;
	int stream_id;
};


struct http2_server_ctx {
	int server_fd;
	int event_fd;
	size_t num_clients;
	struct pollfd client_fds[MAX_CLIENTS+2];
	struct http2_client_ctx clients[MAX_CLIENTS];
	int infinite;
};

/* Initializes the HTTP2 server */
int http2_server_init(struct http2_server_ctx *ctx,
		      struct http2_server_config *config);

/* Starts the HTTP2 server */
int http2_server_start(struct http2_server_ctx *ctx);

/* Stops the HTTP2 server */
int http2_server_stop(struct http2_server_ctx *ctx);

#endif
