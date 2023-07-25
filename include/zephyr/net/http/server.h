/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_SERVER_H_
#define ZEPHYR_INCLUDE_NET_HTTP_SERVER_H_

#include <stdint.h>

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)

#include <stddef.h>
#include <sys/socket.h>
#include <poll.h>

#else

#include <zephyr/posix/poll.h>

#endif

#if !defined(__ZEPHYR__)

#define CONFIG_NET_HTTP_SERVER_MAX_CLIENTS                10
#define CONFIG_NET_HTTP_SERVER_MAX_STREAMS                100
#define CONFIG_NET_HTTP_SERVER_CLIENT_BUFFER_SIZE         256
#define CONFIG_NET_HTTP_SERVER_POST_REQUEST_STORAGE_LIMIT 100

#endif

#define CLIENT_BUFFER_SIZE         CONFIG_NET_HTTP_SERVER_CLIENT_BUFFER_SIZE
#define MAX_CLIENTS                CONFIG_NET_HTTP_SERVER_MAX_CLIENTS
#define MAX_STREAMS                CONFIG_NET_HTTP_SERVER_MAX_STREAMS
#define POST_REQUEST_STORAGE_LIMIT CONFIG_NET_HTTP_SERVER_POST_REQUEST_STORAGE_LIMIT

struct arithmetic_result {
	int x;
	int y;
	int result;
};

struct http_server_config {
	int port;
	sa_family_t address_family;
};

enum http_stream_state {
	HTTP_SERVER_STREAM_IDLE,
	HTTP_SERVER_STREAM_RESERVED_LOCAL,
	HTTP_SERVER_STREAM_RESERVED_REMOTE,
	HTTP_SERVER_STREAM_OPEN,
	HTTP_SERVER_STREAM_HALF_CLOSED_LOCAL,
	HTTP_SERVER_STREAM_HALF_CLOSED_REMOTE,
	HTTP_SERVER_STREAM_CLOSED
};

enum http_server_state {
	HTTP_SERVER_FRAME_HEADER_STATE,
	HTTP_SERVER_PREFACE_STATE,
	HTTP_SERVER_REQUEST_STATE,
	HTTP_SERVER_FRAME_DATA_STATE,
	HTTP_SERVER_FRAME_HEADERS_STATE,
	HTTP_SERVER_FRAME_SETTINGS_STATE,
	HTTP_SERVER_FRAME_PRIORITY_STATE,
	HTTP_SERVER_FRAME_WINDOW_UPDATE_STATE,
	HTTP_SERVER_FRAME_CONTINUATION_STATE,
	HTTP_SERVER_FRAME_PING_STATE,
	HTTP_SERVER_FRAME_RST_STREAM_STATE,
	HTTP_SERVER_FRAME_GOAWAY_STATE,
	HTTP_SERVER_DONE_STATE,
};

struct http_stream_ctx {
	int stream_id;
	enum http_stream_state stream_state;
};

struct http_frame {
	uint32_t length;
	uint32_t stream_identifier;
	uint8_t type;
	uint8_t flags;
	uint8_t *payload;
};

struct http_client_ctx {
	int client_fd;
	int offset;
	unsigned char buffer[CLIENT_BUFFER_SIZE];
	enum http_server_state server_state;
	struct http_frame current_frame;
	struct http_stream_ctx streams[MAX_STREAMS];
};

struct http_server_ctx {
	int server_fd;
	int event_fd;
	size_t num_clients;
	int infinite;
	struct http_server_config config;
	struct pollfd fds[MAX_CLIENTS + 2];
	struct http_client_ctx clients[MAX_CLIENTS];
	struct arithmetic_result results[POST_REQUEST_STORAGE_LIMIT];
	int results_count;
};

/* Initializes the HTTP2 server */
int http_server_init(struct http_server_ctx *ctx);

/* Starts the HTTP2 server */
int http_server_start(struct http_server_ctx *ctx);

/* Stops the HTTP2 server */
int http_server_stop(struct http_server_ctx *ctx);

#endif
