/*
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HTTP_SERVER_INTERNAL_H_
#define HTTP_SERVER_INTERNAL_H_

#include <stdbool.h>

#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/http/status.h>
#include <zephyr/net/http/hpack.h>
#include <zephyr/net/http/frame.h>

#define INVALID_SOCK -1
/* Stream helper consumed or intentionally ignored the fd. */
#define H3_STREAM_IGNORED 1

/* HTTP1/HTTP2 state handling */
int handle_http_frame_rst_stream(struct http_client_ctx *client);
int handle_http_frame_goaway(struct http_client_ctx *client);
int handle_http_frame_settings(struct http_client_ctx *client);
int handle_http_frame_priority(struct http_client_ctx *client);
int handle_http_frame_continuation(struct http_client_ctx *client);
int handle_http_frame_window_update(struct http_client_ctx *client);
int handle_http_frame_header(struct http_client_ctx *client);
int handle_http_frame_headers(struct http_client_ctx *client);
int handle_http_frame_data(struct http_client_ctx *client);
int handle_http_frame_padding(struct http_client_ctx *client);
int handle_http1_request(struct http_client_ctx *client);
int handle_http1_to_http2_upgrade(struct http_client_ctx *client);
int handle_http1_to_websocket_upgrade(struct http_client_ctx *client);
void http_server_release_client(struct http_client_ctx *client);

int enter_http1_request(struct http_client_ctx *client);
int enter_http2_request(struct http_client_ctx *client);
int enter_http_done_state(struct http_client_ctx *client);

/* HTTP/3 state handling */
/**
 * HTTP/3 peer settings received from SETTINGS frame.
 */
struct h3_peer_settings {
	uint64_t qpack_max_table_capacity;  /* Default: 0 */
	uint64_t max_field_section_size;    /* Default: unlimited (UINT64_MAX) */
	uint64_t qpack_blocked_streams;     /* Default: 0 */
	bool received;                      /* True if SETTINGS has been received */
};

/**
 * HTTP/3 connection context for tracking unidirectional streams.
 * This is stored per H3 connection (QUIC context socket).
 */
struct h3_conn_ctx {
	/* Peer's unidirectional streams (accepted by us) */
	int peer_control_stream;
	int peer_qpack_encoder_stream;
	int peer_qpack_decoder_stream;

	/* Our unidirectional streams (opened by us) */
	int local_control_stream;
	int local_qpack_encoder_stream;
	int local_qpack_decoder_stream;

	/* Peer settings */
	struct h3_peer_settings peer_settings;

	/* State flags */
	bool settings_sent;
	bool initialized;
};

int accept_h3_connection(int h3_listen_sock);
int accept_h3_stream(int h3_conn_sock, int *stream_sock);
int handle_http3_request(struct http_client_ctx *client);
int handle_http3_stream_fin(struct http_client_ctx *client);
int h3_open_uni_streams(struct http_client_ctx *client, int quic_sock);
int h3_identify_uni_stream(struct http_client_ctx *client, int fd);
int h3_handle_control_stream(struct http_client_ctx *client, int fd);
int h3_handle_qpack_encoder_stream(struct http_client_ctx *client, int fd);
int h3_handle_qpack_decoder_stream(struct http_client_ctx *client, int fd);
struct http_client_ctx *h3_find_client(int conn_sock, int *idx);
int h3_client_cleanup(struct http_client_ctx *client,
		      struct zsock_pollfd fds[],
		      int max_fds_count);
int h3_handle_uni_stream_data(struct http_client_ctx *client, int fd);
bool h3_is_unidirectional_stream(int fd);
struct http_client_ctx *h3_find_client_for_uni_stream(int fd);
struct h3_conn_ctx *h3_get_conn_ctx(struct http_client_ctx *client);

/* HTTP Compression handling */
#define HTTP_COMPRESSION_MAX_STRING_LEN 8
void http_compression_parse_accept_encoding(const char *accept_encoding, size_t len,
					    uint8_t *supported_compression);
const char *http_compression_text(enum http_compression compression);
int http_compression_from_text(enum http_compression *compression, const char *text);
bool compression_value_is_valid(enum http_compression compression);

/* Others */
struct http_resource_detail *get_resource_detail(const struct http_service_desc *service,
						 const char *path, int *len, bool is_ws);
void http_server_remove_dot_segments(char *path);
int http_server_sendall(struct http_client_ctx *client, const void *buf, size_t len);
void http_server_get_content_type_from_extension(char *url, char *content_type,
						 size_t content_type_size);
int http_server_find_file(char *fname, size_t fname_size, size_t *file_size,
			  uint8_t supported_compression, enum http_compression *chosen_compression);
void http_client_timer_restart(struct http_client_ctx *client);
bool http_response_is_final(struct http_response_ctx *rsp, enum http_transaction_status status);
bool http_response_is_provided(struct http_response_ctx *rsp);

/* TODO Could be static, but currently used in tests. */
int parse_http_frame_header(struct http_client_ctx *client, const uint8_t *buffer, size_t buflen);
const char *get_frame_type_name(enum http2_frame_type type);

void populate_request_ctx(struct http_request_ctx *req_ctx, uint8_t *data, size_t len,
			  struct http_header_capture_ctx *header_ctx);

#endif /* HTTP_SERVER_INTERNAL_H_ */
