/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HTTP_SERVER_FUNCTIONS_H_
#define HTTP_SERVER_FUNCTIONS_H_

#include <stdbool.h>

#include <zephyr/net/http/parser.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/hpack.h>
#include <zephyr/net/http/frame.h>

void generate_response_headers_frame(unsigned char *response_headers_frame, int new_stream_id,
				     int hpack_status);
void send_data(int socket_fd, const char *payload, size_t length, uint8_t type, uint8_t flags,
	       uint32_t stream_id);
void print_http_frames(struct http_client_ctx *ctx_client);
int parse_http_frame_header(struct http_client_ctx *ctx_client);
bool settings_ack_flag(unsigned char flags);
bool settings_end_headers_flag(unsigned char flags);
bool settings_end_stream_flag(unsigned char flags);
ssize_t sendall(int sock, const void *buf, size_t len);
const char *get_frame_type_name(enum http_frame_type type);
int on_header_field(struct http_parser *p, const char *at, size_t length);
int on_url(struct http_parser *p, const char *at, size_t length);
int accept_new_client(int server_fd);
void initialize_client_ctx(struct http_client_ctx *client, int new_socket);
void close_client_connection(struct http_server_ctx *ctx, int client_index);
int handle_http_preface(struct http_client_ctx *ctx_client);
int handle_http_request(struct http_server_ctx *ctx_server, struct http_client_ctx *ctx_client,
			int client_index);
int handle_http_frame_rst_frame(struct http_server_ctx *ctx_server,
				struct http_client_ctx *ctx_client, int client_index);
int handle_http_frame_goaway(struct http_server_ctx *ctx_server, struct http_client_ctx *ctx_client,
			     int client_index);
int handle_http_frame_settings(struct http_client_ctx *ctx_client);
int handle_http_frame_priority(struct http_client_ctx *ctx_client);
int handle_http_frame_continuation(struct http_client_ctx *ctx_client);
int handle_http_frame_window_update(struct http_client_ctx *ctx_client);
int handle_http_frame_header(struct http_server_ctx *ctx_server, struct http_client_ctx *ctx_client,
			     int client_index);
int handle_http_frame_headers(struct http_client_ctx *ctx_client);
int handle_http1_request(struct http_server_ctx *ctx_server, struct http_client_ctx *ctx_client,
			 int client_index);
int handle_http_done(struct http_server_ctx *ctx_server, struct http_client_ctx *ctx_client,
		     int client_index);
int enter_http_frame_headers_state(struct http_server_ctx *ctx_server,
				   struct http_client_ctx *ctx_client, int client_index);
int enter_http_frame_continuation_state(struct http_client_ctx *ctx_client);
int enter_http_frame_window_update_state(struct http_client_ctx *ctx_client);
int enter_http_frame_settings_state(struct http_client_ctx *ctx_client);
int enter_http_frame_priority_state(struct http_client_ctx *ctx_client);
int enter_http_frame_goaway_state(struct http_server_ctx *ctx_server,
				  struct http_client_ctx *ctx_client, int client_index);
int enter_http_http_done_state(struct http_server_ctx *ctx_server,
			       struct http_client_ctx *ctx_client, int client_index);
int enter_http_frame_rst_stream_state(struct http_server_ctx *ctx_server,
				      struct http_client_ctx *ctx_client, int client_index);
struct http_stream_ctx *find_http_stream_context(struct http_client_ctx *ctx_client,
						 uint32_t stream_id);
struct http_stream_ctx *allocate_http_stream_context(struct http_client_ctx *ctx_client,
						     uint32_t stream_id);
void handle_post_request(struct http_server_ctx *ctx_server, char *request_payload);
void handle_get_request(struct http_server_ctx *ctx_server, int client);

#endif
