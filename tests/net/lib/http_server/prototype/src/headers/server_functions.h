#ifndef SERVER_FUNCTIONS_H
#define SERVER_FUNCTIONS_H

#include <zephyr/net/http/parser.h>

int on_url(struct http_parser *parser, const char *at, size_t length);
ssize_t sendall(int sock, const void *buf, size_t len);
void close_client_connection(struct http2_server_ctx *ctx, int clientIndex);
int accept_new_client(int server_fd);
void handle_http1_request(struct http2_server_ctx *ctx, int i);
void handle_http2_request(struct http2_server_ctx *ctx, int i, int valread);
int on_header_field(struct http_parser *parser, const char *at, size_t length);
void generate_response_headers_frame(unsigned char *response_headers_frame, int new_stream_id);
void sendData(int socket_fd, const char *payload, size_t length, uint8_t type, uint8_t flags, uint32_t streamId);
void print_http2_frames(struct http2_frame *frames, unsigned int frame_count);
unsigned int parse_http2_frames(unsigned char *buffer, unsigned long buffer_len, struct http2_frame *frames);
int find_headers_frame_stream_id(struct http2_frame *frames, unsigned int frame_count);

#endif
