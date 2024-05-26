/*
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_SERVER_H_
#define ZEPHYR_INCLUDE_NET_HTTP_SERVER_H_

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/net/http/parser.h>
#include <zephyr/net/http/hpack.h>
#include <zephyr/net/socket.h>

#define HTTP_SERVER_CLIENT_BUFFER_SIZE CONFIG_HTTP_SERVER_CLIENT_BUFFER_SIZE
#define HTTP_SERVER_MAX_STREAMS        CONFIG_HTTP_SERVER_MAX_STREAMS
#define HTTP_SERVER_MAX_CONTENT_TYPE_LEN CONFIG_HTTP_SERVER_MAX_CONTENT_TYPE_LENGTH

/* Maximum header field name / value length. This is only used to detect Upgrade and
 * websocket header fields and values in the http1 server so the value is quite short.
 */
#define HTTP_SERVER_MAX_HEADER_LEN 32

#define HTTP2_PREFACE "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"

enum http_resource_type {
	HTTP_RESOURCE_TYPE_STATIC,
	HTTP_RESOURCE_TYPE_DYNAMIC,
};

struct http_resource_detail {
	uint32_t bitmask_of_supported_http_methods;
	enum http_resource_type type;
	int path_len; /* length of the URL path */
	const char *content_encoding;
};
BUILD_ASSERT(NUM_BITS(
	     sizeof(((struct http_resource_detail *)0)->bitmask_of_supported_http_methods))
	     >= (HTTP_METHOD_END_VALUE - 1));

struct http_resource_detail_static {
	struct http_resource_detail common;
	const void *static_data;
	size_t static_data_len;
};

struct http_client_ctx;

/** Indicates the status of the currently processed piece of data.  */
enum http_data_status {
	/** Transaction aborted, data incomplete. */
	HTTP_SERVER_DATA_ABORTED = -1,
	/** Transaction incomplete, more data expected. */
	HTTP_SERVER_DATA_MORE = 0,
	/** Final data fragment in current transaction. */
	HTTP_SERVER_DATA_FINAL = 1,
};

/**
 * @typedef http_resource_dynamic_cb_t
 * @brief Callback used when data is received. Data to be sent to client
 *        can be specified.
 *
 * @param client HTTP context information for this client connection.
 * @param status HTTP data status, indicate whether more data is expected or not.
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
					  enum http_data_status status,
					  uint8_t *data_buffer,
					  size_t data_len,
					  void *user_data);

struct http_resource_detail_dynamic {
	struct http_resource_detail common;
	http_resource_dynamic_cb_t cb;
	uint8_t *data_buffer;
	size_t data_buffer_len;
	struct http_client_ctx *holder;
	void *user_data;
};

struct http_resource_detail_rest {
	struct http_resource_detail common;
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

enum http1_parser_state {
	HTTP1_INIT_HEADER_STATE,
	HTTP1_WAITING_HEADER_STATE,
	HTTP1_RECEIVING_HEADER_STATE,
	HTTP1_RECEIVED_HEADER_STATE,
	HTTP1_RECEIVING_DATA_STATE,
	HTTP1_MESSAGE_COMPLETE_STATE,
};

#define HTTP_SERVER_INITIAL_WINDOW_SIZE 65536

struct http_stream_ctx {
	int stream_id;
	enum http_stream_state stream_state;
	int window_size; /**< Stream-level window size. */
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
	unsigned char buffer[HTTP_SERVER_CLIENT_BUFFER_SIZE];
	unsigned char *cursor; /**< Cursor indicating currently processed byte. */
	size_t data_len; /**< Data left to process in the buffer. */
	int window_size; /**< Connection-level window size.  */
	enum http_server_state server_state;
	struct http_frame current_frame;
	struct http_resource_detail *current_detail;
	struct http_hpack_header_buf header_field;
	struct http_stream_ctx streams[HTTP_SERVER_MAX_STREAMS];
	struct http_parser_settings parser_settings;
	struct http_parser parser;
	unsigned char url_buffer[CONFIG_HTTP_SERVER_MAX_URL_LENGTH];
	unsigned char content_type[CONFIG_HTTP_SERVER_MAX_CONTENT_TYPE_LENGTH];
	unsigned char header_buffer[HTTP_SERVER_MAX_HEADER_LEN];
	size_t content_len;
	enum http_method method;
	enum http1_parser_state parser_state;
	int http1_frag_data_len;
	struct k_work_delayable inactivity_timer;
	bool headers_sent : 1;
	bool preface_sent : 1;
	bool has_upgrade_header : 1;
	bool http2_upgrade : 1;
	bool websocket_upgrade : 1;
};

/* Starts the HTTP2 server */
int http_server_start(void);

/* Stops the HTTP2 server */
int http_server_stop(void);

#endif
