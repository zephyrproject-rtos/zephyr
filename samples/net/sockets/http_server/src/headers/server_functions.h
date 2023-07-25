/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SERVER_FUNCTIONS_H
#define SERVER_FUNCTIONS_H

#include <stdbool.h>

#include <zephyr/net/http/parser.h>
#include <zephyr/net/http/server.h>
#include "config.h"

void generate_response_headers_frame(
	unsigned char *response_headers_frame, int new_stream_id);
void send_data(int socket_fd, const char *payload, size_t length, uint8_t type,
	uint8_t flags, uint32_t stream_id);
void print_http2_frames(struct http2_frame *frame);
int parse_http2_frame(unsigned char *buffer, unsigned long buffer_len, struct http2_frame *frame);
int find_headers_frame_stream_id(struct http2_frame *frame);
enum http2_server_state determine_server_state(unsigned char *buffer);
enum http2_streaming_state determine_stream_state(unsigned char *buffer);
unsigned int determine_frame_size(unsigned char *buffer);
bool settings_ack_flag(unsigned char flags);
bool settings_end_headers_flag(unsigned char flags);
bool settings_stream_flag(unsigned char flags);
ssize_t sendall(int sock, const void *buf, size_t len);
const char *get_frame_type_name(enum http2_frame_type type);
int on_header_field(struct http_parser *parser, const char *at, size_t length);
int on_url(struct http_parser *parser, const char *at, size_t length);
int http2_server_init(
	struct http2_server_ctx *ctx, struct http2_server_config *config);
int accept_new_client(int server_fd);
void initialize_client_ctx(struct http2_client_ctx *client, int new_socket);
int http2_server_start(struct http2_server_ctx *ctx);
void close_client_connection(struct http2_server_ctx *ctx, int client_index);
int handle_http_preface(struct http2_client_ctx *ctx_client);
int handle_http1_request(struct http2_server_ctx *ctx_server,
	struct http2_client_ctx *ctx_client, int client_index);
int handle_http2_frame_rst_frame(struct http2_server_ctx *ctx_server,
	struct http2_client_ctx *ctx_client, int client_index);
int handle_http2_frame_goaway(struct http2_server_ctx *ctx_server,
	struct http2_client_ctx *ctx_client, int client_index);
int handle_http2_frame_setting(struct http2_client_ctx *ctx_client);
int handle_http2_frame_window_update(struct http2_client_ctx *ctx_client);
int handle_http2_idle_state(struct http2_client_ctx *ctx_client);
int handle_http2_open_state(struct http2_client_ctx *ctx_client);
int handle_http2_frame_headers(struct http2_client_ctx *ctx_client);
int handle_http_request(struct http2_server_ctx *ctx_server,
	struct http2_client_ctx *ctx_client, int client_index);

#endif
