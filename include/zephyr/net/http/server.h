/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_SERVER_H_
#define ZEPHYR_INCLUDE_NET_HTTP_SERVER_H_

#include <stdint.h>

#if defined(CONFIG_POSIX_API)

#include <stddef.h>
#include <poll.h>

#else

#include <zephyr/posix/poll.h>

#endif

#include <zephyr/net/http/parser.h>

#define CLIENT_BUFFER_SIZE         CONFIG_NET_HTTP_SERVER_CLIENT_BUFFER_SIZE
#define MAX_SERVICES               CONFIG_NUM_HTTP_SERVICES
#define MAX_CLIENTS                CONFIG_NET_HTTP_SERVER_MAX_CLIENTS
#define MAX_STREAMS                CONFIG_NET_HTTP_SERVER_MAX_STREAMS

enum http_resource_type {
	HTTP_RESOURCE_TYPE_STATIC,
	HTTP_RESOURCE_TYPE_DYNAMIC,
	HTTP_RESOURCE_TYPE_REST,
	HTTP_RESOURCE_TYPE_REST_JSON
};

struct http_resource_detail {
	uint32_t bitmask_of_supported_http_methods;
	enum http_resource_type type;
	int path_len; /* length of the URL path */
};
BUILD_ASSERT(NUM_BITS(\
	     sizeof(((struct http_resource_detail *)0)->bitmask_of_supported_http_methods))
	     >= (HTTP_METHOD_END_VALUE - 1));

struct http_resource_detail_static {
	struct http_resource_detail common;
	const void *static_data;
	size_t static_data_len;
};

struct http_client_ctx;

/**
 * @typedef http_resource_dynamic_cb_t
 * @brief Callback used when data is received. Data to be sent to client
 *        can be specified.
 *
 * @param client HTTP context information for this client connection.
 * @param data_buffer Data received.
 * @param data_len Amount of data received.
 * @param user_data User specified data.
 *
 * @return >0 amount of data to be sent to client, let server to call this
 *            function again when new data is received.
 *          0 nothing to sent to client, close the connection
 *         <0 error, close the connection.
 */
typedef int (*http_resource_dynamic_cb_t)(struct http_client_ctx *client,
					  uint8_t *data_buffer,
					  size_t data_len,
					  void *user_data);

struct http_resource_detail_dynamic {
	struct http_resource_detail common;
	http_resource_dynamic_cb_t cb;
	uint8_t *data_buffer;
	size_t data_buffer_len;
	void *user_data;
};

struct http_resource_detail_rest {
	struct http_resource_detail common;
};

struct http_resource_detail_rest_json {
	struct http_resource_detail common;
	http_resource_dynamic_cb_t cb;
	uint8_t *data_buffer;
	size_t data_buffer_len;
	const struct json_obj_descr *descr;
	size_t descr_len;
	void *json;
	void *user_data;
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
	int fd;
	int offset;
	bool has_upgrade_header;
	unsigned char buffer[CLIENT_BUFFER_SIZE];
	enum http_server_state server_state;
	struct http_frame current_frame;
	struct http_stream_ctx streams[MAX_STREAMS];
	struct http_parser_settings parser_settings;
	struct http_parser parser;
	unsigned char url_buffer[CONFIG_NET_HTTP_SERVER_MAX_URL_LENGTH];
};

struct http_server_ctx {
	int num_clients;
	int listen_fds;   /* max value of 1 + MAX_SERVICES */

	/* First pollfd is eventfd that can be used to stop the server,
	 * then we have the server listen sockets,
	 * and then the accepted sockets.
	 */
	struct pollfd fds[1 + MAX_SERVICES + MAX_CLIENTS];
	struct http_client_ctx clients[MAX_CLIENTS];
};

/* Initializes the HTTP2 server */
int http_server_init(struct http_server_ctx *ctx);

/* Starts the HTTP2 server */
int http_server_start(struct http_server_ctx *ctx);

/* Stops the HTTP2 server */
int http_server_stop(struct http_server_ctx *ctx);

#endif
