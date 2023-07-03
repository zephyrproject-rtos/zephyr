#ifndef SERVER_FUNCTIONS_H
#define SERVER_FUNCTIONS_H

#include "config.h"
#include <zephyr/net/http/parser.h>


int http2_server_init(struct http2_server_ctx *ctx, struct http2_server_config *config);
int http2_server_start(int server_fd, struct http2_server_ctx *ctx);
int on_url(struct http_parser *parser, const char *at, size_t length);
ssize_t sendall(int sock, const void *buf, size_t len);
void close_client_connection(struct http2_server_ctx *ctx, int clientIndex);
int accept_new_client(int server_fd);
void handle_client_data(int i, char *buffer);
void handle_incoming_clients(int server_fd, struct sockaddr_in *address, int *addrlen);
void handle_http1_request(struct http2_server_ctx *ctx, int i);
void handle_http2_request(struct http2_server_ctx *ctx, int i, int valread);
int on_header_field(struct http_parser *parser, const char *at, size_t length);
void generate_response_headers_frame(unsigned char *response_headers_frame, int new_stream_id);
void sendData(int socket_fd, const char *payload, size_t length, uint8_t type, uint8_t flags, uint32_t streamId);
void print_http2_frames(Http2Frame *frames, unsigned int frame_count);
unsigned int parse_http2_frames(unsigned char *buffer, unsigned long buffer_len, Http2Frame *frames);
int find_headers_frame_stream_id(Http2Frame *frames, unsigned int frame_count);

#endif
