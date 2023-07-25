/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "headers/server_functions.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#else

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/sys/eventfd.h>

#endif


#if defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_http_server, LOG_LEVEL_DBG);

#elif defined(__linux__)

#include "headers/mlog.h"

#endif

static char url_buffer[MAX_HTTP_URL_LENGTH];
static char http_response[MAX_HTTP_RESPONSE_SIZE];
static const char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

struct http_parser_settings parserSettings;
struct http_parser parser;

static unsigned char settings_frame[9] = {0x00, 0x00, 0x00, /* Length */
	0x04, /* Type: 0x04 - setting frames for config or acknowledgment */
	0x00, /* Flags: 0x00 - unused flags */
	0x00, 0x00, 0x00,
	0x00}; /* Reserved, Stream Identifier: 0x00 - overall connection */

static unsigned char settings_ack[9] = {0x00, 0x00, 0x00, /* Length */
	0x04, /* Type: 0x04 - setting frames for config or acknowledgment */
	0x01, /* Flags: 0x01 - ACK */
	0x00, 0x00, 0x00, 0x00}; /* Reserved, Stream Identifier */

static struct http2_frame frame;

const char content[] = {
#include "index.html.gz.inc"
};

bool has_upgrade_header = true;

int http2_server_init(
	struct http2_server_ctx *ctx, struct http2_server_config *config)
{
	/* Create a socket */
	ctx->server_fd = socket(config->address_family, SOCK_STREAM, 0);
	if (ctx->server_fd < 0) {
		LOG_ERR("socket");
		return ctx->server_fd;
	}

	if (setsockopt(ctx->server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1},
		    sizeof(int)) < 0) {
		LOG_ERR("setsockopt");
		return -errno;
	}

	/* Set up the server address struct according to address family */
	if (config->address_family == AF_INET) {
		struct sockaddr_in serv_addr;

		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(config->port);

		if (bind(ctx->server_fd, (struct sockaddr *)&serv_addr,
			    sizeof(serv_addr)) < 0) {
			LOG_ERR("bind");
			return -errno;
		}
	} else if (config->address_family == AF_INET6) {
		struct sockaddr_in6 serv_addr;

		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin6_family = AF_INET6;
		serv_addr.sin6_addr = in6addr_any;
		serv_addr.sin6_port = htons(config->port);

		if (bind(ctx->server_fd, (struct sockaddr *)&serv_addr,
			    sizeof(serv_addr)) < 0) {
			LOG_ERR("bind");
			return -errno;
		}
	}

	/* Listen for connections */
	if (listen(ctx->server_fd, MAX_CLIENTS) < 0) {
		LOG_ERR("listen");
		return -errno;
	}

	/* Create an eventfd*/
	ctx->event_fd = eventfd(0, 0);
	if (ctx->event_fd < 0) {
		LOG_ERR("eventfd");
		return -errno;
	}

	/* Initialize fds */
	memset(ctx->fds, 0, sizeof(ctx->fds));
	memset(ctx->clients, 0, sizeof(ctx->clients));

	ctx->fds[0].fd = ctx->server_fd;
	ctx->fds[0].events = POLLIN;

	ctx->fds[1].fd = ctx->event_fd;
	ctx->fds[1].events = POLLIN;

	ctx->num_clients = 0;
	ctx->infinite = 1;

	return ctx->server_fd;
}

int accept_new_client(int server_fd)
{
	int new_socket;

	socklen_t addrlen;
	struct sockaddr_in sa;

	memset(&sa, 0, sizeof(sa));
	addrlen = sizeof(sa);

	new_socket = accept(server_fd, (struct sockaddr *)&sa, &addrlen);
	if (new_socket < 0) {
		LOG_ERR("accept failed");
		return new_socket;
	}

	return new_socket;
}

int http2_server_start(struct http2_server_ctx *ctx)
{
	printf("\nType 'quit' to quit\n\n");
	printf("Waiting for incoming connections...\n");

	eventfd_t value;

	do {
		int ret = poll(ctx->fds, ctx->num_clients + 2, 0);

		if (ret < 0) {
			LOG_ERR("poll failed");
			return -errno;
		}

		for (int i = 0; i < ctx->num_clients + 2; i++) {
			struct http2_client_ctx *client = &ctx->clients[i - 2];

			if (ctx->fds[i].revents & POLLERR) {
				LOG_ERR("Error on fd %d\n", ctx->fds[i].fd);
				close_client_connection(ctx, i);
				continue;
			}

			if (ctx->fds[i].revents & POLLHUP) {
				LOG_INF("Client on fd %d has disconnected\n",
					ctx->fds[i].fd);
				close_client_connection(ctx, i);
				continue;
			}

			if (!(ctx->fds[i].revents & POLLIN))
				continue;

			if (i == 0) {
				int new_socket =
					accept_new_client(ctx->server_fd);

				bool found_slot = false;

				for (int j = 2; j < MAX_CLIENTS + 1; j++) {
					if (ctx->fds[j].fd != 0) {
						continue;
					}

					ctx->fds[j].fd = new_socket;
					ctx->fds[j].events = POLLIN;

					initialize_client_ctx(
						&ctx->clients[j - 2],
						new_socket);

					if (j > ctx->num_clients) {
						ctx->num_clients++;
					}

					found_slot = true;
					break;
				}
				if (!found_slot) {
					LOG_INF("No free slot found.\n");
					close(new_socket);
				}
				continue;
			}

			if (i == 1) {
#ifdef __ZEPHYR__
				eventfd_read(ctx->event_fd, &value);
#else
				read(ctx->event_fd, &value, sizeof(value));
#endif
				printf("Received stop event. exiting ..\n");

				return 0;
			}

			int valread = recv(client->client_fd,
				client->buffer + client->offset,
				sizeof(client->buffer) - client->offset, 0);

			if (valread < 0) {
				LOG_ERR("ERROR reading from socket");
				close_client_connection(ctx, i);
				continue;
			}

			if (valread == 0) {
				LOG_INF("Connection closed by peer.\n");
				close_client_connection(ctx, i);
				continue;
			}

			client->offset += valread;
			handle_http_request(ctx, client, i);
		}
	} while (ctx->infinite == 1);
	return 0;
}

void close_client_connection(
	struct http2_server_ctx *ctx_server, int client_index)
{
	close(ctx_server->fds[client_index].fd);
	ctx_server->fds[client_index].fd = 0;
	ctx_server->fds[client_index].events = 0;
	ctx_server->fds[client_index].revents = 0;

	if (client_index == ctx_server->num_clients) {
		while (ctx_server->num_clients > 0 &&
			ctx_server->fds[ctx_server->num_clients].fd == 0) {
			--ctx_server->num_clients;
		}
	}
}

void initialize_client_ctx(struct http2_client_ctx *client, int new_socket)
{
	client->client_fd = new_socket;
	client->offset = 0;
	client->stream_count = 0;
	client->streams[0].stream_state = -1;
	client->server_state = HTTP_PREFACE_STATE;
}

int handle_http2_frame_header(struct http2_client_ctx *ctx_client)
{
	printf("HTTP2_FRAME_HEADER\n");

	if (ctx_client->offset < 9) {
		return -EAGAIN;
	}

	uint32_t length = (ctx_client->buffer[0] << 16) |
			  (ctx_client->buffer[1] << 8) | ctx_client->buffer[2];

	if (ctx_client->offset < length + 9) {
		return -EAGAIN;
	}

	ctx_client->server_state = determine_server_state(ctx_client->buffer);

	return 0;
}

int handle_http2_done(struct http2_server_ctx *ctx_server,
	struct http2_client_ctx *ctx_client, int client_index)
{
	printf("HTTP_DONE_STATE\n");

	close_client_connection(ctx_server, client_index);

	return -1;
}

int handle_http2_idle_state(struct http2_client_ctx *ctx_client)
{

	printf("IDLE_STATE\n");

	parse_http2_frame(ctx_client->buffer, ctx_client->offset, &frame);
	print_http2_frames(&frame);

	ctx_client->streams[ctx_client->stream_count].stream_id =
		find_headers_frame_stream_id(&frame);

	printf("||stream id %d||\n",
		ctx_client->streams[ctx_client->stream_count].stream_id);

	if (!has_upgrade_header) {
		ctx_client->streams[0].stream_id = 1;
		unsigned char response_headers_frame[16];

		ctx_client->streams[ctx_client->stream_count].stream_state =
			OPEN_STATE;

		generate_response_headers_frame(response_headers_frame,
			ctx_client->streams[0].stream_id);

		size_t ret = sendall(ctx_client->client_fd,
			response_headers_frame, sizeof(response_headers_frame));
		if (ret < 0) {
			LOG_ERR("ERROR writing to socket");
			return -errno;
		}

		ctx_client->stream_count = 1;
	}

	if (settings_end_headers_flag(ctx_client->buffer[4])) {
		unsigned char response_headers_frame[16];

		generate_response_headers_frame(response_headers_frame,
			ctx_client->streams[ctx_client->stream_count]
				.stream_id);

		ctx_client->stream_count++;

		size_t ret = sendall(ctx_client->client_fd,
			response_headers_frame, sizeof(response_headers_frame));
		if (ret < 0) {
			LOG_ERR("ERROR writing to socket");
			return -errno;
		}
	} else {
		/* We expect Continuation frame */
		ctx_client->server_state = HTTP2_FRAME_HEADER_STATE;
		return 0;
	}

	unsigned int frame_size = determine_frame_size(ctx_client->buffer);

	ctx_client->offset -= frame_size;
	memmove(ctx_client->buffer, ctx_client->buffer + frame_size,
		ctx_client->offset);

	if (ctx_client->offset == 0) {
		if (ctx_client->streams[ctx_client->stream_count]
				.stream_state == IDLE_STATE) {
			handle_http2_open_state(ctx_client);
		} else {
			ctx_client->server_state = HTTP2_FRAME_HEADER_STATE;
		}
	} else {
		ctx_client->server_state =
			determine_server_state(ctx_client->buffer);
	}

	return 0;
}

int handle_http2_open_state(struct http2_client_ctx *ctx_client)
{
	printf("OPEN_STATE\n");

	parse_http2_frame(ctx_client->buffer, ctx_client->offset, &frame);
	print_http2_frames(&frame);

	size_t content_size = sizeof(content);

	for (int i = 0; i < ctx_client->stream_count; i++) {
		send_data(ctx_client->client_fd, content, content_size, 0x00,
			0x01, ctx_client->streams[i].stream_id);
	}

	ctx_client->streams[0].stream_state = -1;

	if (has_upgrade_header)
		if (ctx_client->offset == 0) {
			ctx_client->server_state = HTTP2_FRAME_HEADER_STATE;
		} else {
			ctx_client->server_state =
				determine_server_state(ctx_client->buffer);
		}
	else
		ctx_client->server_state = HTTP2_FRAME_GOAWAY_STATE;

	has_upgrade_header = true;

	return 0;
}

int handle_http2_frame_headers(struct http2_client_ctx *ctx_client)
{
	printf("HTTP2_FRAME_HEADERS\n");

	ctx_client->streams[0].stream_state =
		determine_stream_state(ctx_client->buffer);
	int ret = -EINVAL;

	switch (ctx_client->streams[0].stream_state) {
	case IDLE_STATE:
		ret = handle_http2_idle_state(ctx_client);
		break;
	case OPEN_STATE:
		ret = handle_http2_open_state(ctx_client);
		break;
	case CLOSE_STATE:
		break;
	default:
		LOG_ERR("Unknown state.\n");
	}

	return 0;
}

int handle_http2_frame_priority(struct http2_client_ctx *ctx_client)
{
	printf("HTTP2_FRAME_PRIORITY_STATE\n");

	unsigned int frame_size = determine_frame_size(ctx_client->buffer);

	ctx_client->offset -= frame_size;
	memmove(ctx_client->buffer, ctx_client->buffer + frame_size,
		ctx_client->offset);

	if (ctx_client->offset == 0) {
		ctx_client->server_state = HTTP2_FRAME_HEADER_STATE;
	} else {
		ctx_client->server_state =
			determine_server_state(ctx_client->buffer);
	}

	return 0;
}

int handle_http2_frame_continuation(struct http2_client_ctx *ctx_client)
{
	printf("HTTP2_FRAME_CONTINUATION_STATE\n");
	ctx_client->server_state = HTTP2_FRAME_HEADERS_STATE;

	return 0;
}

int handle_http_request(struct http2_server_ctx *ctx_server,
	struct http2_client_ctx *ctx_client, int client_index)
{
	int ret = -EINVAL;

	do {
		switch (ctx_client->server_state) {
		case HTTP_PREFACE_STATE:
			ret = handle_http_preface(ctx_client);
			break;
		case HTTP1_REQUEST_STATE:
			ret = handle_http1_request(
				ctx_server, ctx_client, client_index);
			break;
		case HTTP2_FRAME_HEADER_STATE:
			ret = handle_http2_frame_header(ctx_client);
			break;
		case HTTP2_FRAME_HEADERS_STATE:
			ret = handle_http2_frame_headers(ctx_client);
			break;
		case HTTP2_FRAME_CONTINUATION_STATE:
			ret = handle_http2_frame_continuation(ctx_client);
			break;
		case HTTP2_FRAME_SETTINGS_STATE:
			ret = handle_http2_frame_setting(ctx_client);
			break;
		case HTTP2_FRAME_WINDOW_UPDATE_STATE:
			ret = handle_http2_frame_window_update(ctx_client);
			break;
		case HTTP2_FRAME_RST_STREAM_STATE:
			ret = handle_http2_frame_rst_frame(
				ctx_server, ctx_client, client_index);
		case HTTP2_FRAME_GOAWAY_STATE:
			ret = handle_http2_frame_goaway(
				ctx_server, ctx_client, client_index);
			break;
		case HTTP2_FRAME_PRIORITY_STATE:
			ret = handle_http2_frame_priority(ctx_client);
			break;
		case HTTP_DONE_STATE:
			ret = handle_http2_done(
				ctx_server, ctx_client, client_index);
			break;
		default:
			LOG_ERR("Unknown state.\n");
		}

	} while (ret == 0 && ctx_client->offset > 0);

	return ret;
}

int handle_http_preface(struct http2_client_ctx *ctx_client)
{
	printf("HTTP_PREFACE_STATE.\n");
	if (ctx_client->offset < strlen(preface)) {
		/* We don't have full preface yet, get more data. */
		return -EAGAIN;
	}

	if (strncmp(ctx_client->buffer, preface, strlen(preface)) != 0) {
		ctx_client->server_state = HTTP1_REQUEST_STATE;
	} else {
		ctx_client->server_state = HTTP2_FRAME_HEADER_STATE;

		ctx_client->offset -= strlen(preface);
		memmove(ctx_client->buffer,
			ctx_client->buffer + strlen(preface),
			ctx_client->offset);
	}

	return 0;
}

int handle_http1_request(struct http2_server_ctx *ctx_server,
	struct http2_client_ctx *ctx_client, int client_index)
{
	printf("HTTP1_REQUEST.\n");
	const char *data;
	int len;
	int total_received = 0;
	int offset = 0;

	http_parser_init(&parser, HTTP_REQUEST);
	http_parser_settings_init(&parserSettings);
	parserSettings.on_header_field = on_header_field;
	http_parser_execute(&parser, &parserSettings,
		ctx_client->buffer + offset, ctx_client->offset);

	total_received += ctx_client->offset;
	offset += ctx_client->offset;

	if (offset >= MAX_HTTP_RESPONSE_SIZE)
		offset = 0;

	if (has_upgrade_header == false) {
		const char *response = "HTTP/1.1 101 Switching Protocols\r\n"
				       "Connection: Upgrade\r\n"
				       "Upgrade: h2c\r\n"
				       "\r\n";
		if (sendall(ctx_client->client_fd, response, strlen(response)) <
			0)
			close_client_connection(ctx_server, client_index);

		memset(ctx_client->buffer, 0, sizeof(ctx_client->buffer));
		ctx_client->offset = 0;
		ctx_client->server_state = HTTP_PREFACE_STATE;

	} else {
		data = content;
		len = sizeof(content);

		http_parser_init(&parser, HTTP_REQUEST);
		http_parser_settings_init(&parserSettings);
		parserSettings.on_url = on_url;
		http_parser_execute(&parser, &parserSettings,
			ctx_client->buffer, sizeof(ctx_client->buffer));

		if (strcmp(url_buffer, "/") == 0) {
			sprintf(http_response,
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/html\r\n"
				"Content-Encoding: gzip\r\n"
				"Content-Length: %d\r\n\r\n",
				len);
			if (sendall(ctx_client->client_fd, http_response,
				    strlen(http_response)) < 0) {
				close_client_connection(
					ctx_server, client_index);
			} else {

				if (sendall(ctx_client->client_fd, data, len) <
					0) {
					LOG_ERR("sendall failed");
					close_client_connection(
						ctx_server, client_index);
				}
			}
		} else {
			const char *not_found_response =
				"HTTP/1.1 404 Not Found\r\n"
				"Content-Length: 9\r\n\r\n"
				"Not Found";
			if (sendall(ctx_client->client_fd, not_found_response,
				    strlen(not_found_response)) < 0) {
				close_client_connection(
					ctx_server, client_index);
			}
		}
		close_client_connection(ctx_server, client_index);
		memset(ctx_client->buffer, 0, sizeof(ctx_client->buffer));
		ctx_client->offset = 0;
	}

	return 0;
}

int handle_http2_frame_setting(struct http2_client_ctx *ctx_client)
{
	printf("HTTP2_FRAME_SETTINGS\n");

	parse_http2_frame(ctx_client->buffer, ctx_client->offset, &frame);
	print_http2_frames(&frame);

	if (!settings_ack_flag(ctx_client->buffer[4])) {
		ssize_t ret = sendall(ctx_client->client_fd, settings_frame,
			sizeof(settings_frame));
		if (ret < 0) {
			LOG_ERR("ERROR writing to socket");
			return -errno;
		}

		ret = sendall(ctx_client->client_fd, settings_ack,
			sizeof(settings_ack));
		if (ret < 0) {
			LOG_ERR("ERROR writing to socket");
			return -errno;
		}
	}

	unsigned int frame_size = determine_frame_size(ctx_client->buffer);

	ctx_client->offset -= frame_size;
	memmove(ctx_client->buffer, ctx_client->buffer + frame_size,
		ctx_client->offset);

	if (ctx_client->offset == 0) {
		if (ctx_client->streams[ctx_client->stream_count]
				.stream_state == OPEN_STATE) {
			handle_http2_open_state(ctx_client);
		} else {
			ctx_client->server_state = HTTP2_FRAME_HEADER_STATE;
		}
	} else {
		ctx_client->server_state =
			determine_server_state(ctx_client->buffer);
	}

	return 0;
}

int handle_http2_frame_window_update(struct http2_client_ctx *ctx_client)
{
	printf("HTTP2_FRAME_WINDOW_UPDATE\n");

	parse_http2_frame(ctx_client->buffer, ctx_client->offset, &frame);
	print_http2_frames(&frame);

	unsigned int frame_size = determine_frame_size(ctx_client->buffer);

	ctx_client->offset -= frame_size;
	memmove(ctx_client->buffer, ctx_client->buffer + frame_size,
		ctx_client->offset);

	if (!has_upgrade_header) {
		ctx_client->streams[0].stream_state = IDLE_STATE;
		ctx_client->stream_count = 1;
		handle_http2_idle_state(ctx_client);
		return 0;
	}

	ctx_client->server_state = HTTP2_FRAME_HEADER_STATE;

	return 0;
}

int handle_http2_frame_goaway(struct http2_server_ctx *ctx_server,
	struct http2_client_ctx *ctx_client, int client_index)
{
	printf("HTTP2_FRAME_GOAWAY\n");

	parse_http2_frame(ctx_client->buffer, ctx_client->offset, &frame);
	print_http2_frames(&frame);

	close_client_connection(ctx_server, client_index);
	has_upgrade_header = true;
	memset(ctx_client->buffer, 0, sizeof(ctx_client->buffer));

	ctx_client->offset = 0;

	return 0;
}

int handle_http2_frame_rst_frame(struct http2_server_ctx *ctx_server,
	struct http2_client_ctx *ctx_client, int client_index)
{
	printf("FRAME_RST_STREAM\n");

	parse_http2_frame(ctx_client->buffer, ctx_client->offset, &frame);
	print_http2_frames(&frame);

	unsigned int frame_size = determine_frame_size(ctx_client->buffer);

	ctx_client->offset -= frame_size;
	memmove(ctx_client->buffer, ctx_client->buffer + frame_size,
		ctx_client->offset);

	if (ctx_client->offset == 0) {
		close_client_connection(ctx_server, client_index);
		has_upgrade_header = true;
		memset(ctx_client->buffer, 0, sizeof(ctx_client->buffer));

		ctx_client->server_state = HTTP2_FRAME_HEADER_STATE;
	} else {
		ctx_client->server_state =
			determine_server_state(ctx_client->buffer);
	}

	return 0;
}

int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
	if (length == 7 && strncasecmp(at, "Upgrade", length) == 0) {
		LOG_INF("The \"Upgrade: h2c\" header is present.\n");
		has_upgrade_header = false;
	}
	return 0;
}

int on_url(struct http_parser *parser, const char *at, size_t length)
{
	strncpy(url_buffer, at, length);
	url_buffer[length] = '\0';
	printf("Requested URL: %s\n", url_buffer);
	return 0;
}

ssize_t sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = send(sock, buf, len, 0);

		if (out_len < 0)
			return out_len;

		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

void generate_response_headers_frame(
	unsigned char *response_headers_frame, int new_stream_id)
{
	response_headers_frame[0] = 0x00;
	response_headers_frame[1] = 0x00;
	response_headers_frame[2] = 0x07;
	response_headers_frame[3] = 0x01;
	response_headers_frame[4] = 0x04; /* Flags: END_HEADERS */
	response_headers_frame[5] = 0x00;
	response_headers_frame[6] = 0x00;
	response_headers_frame[7] = 0x00;
	response_headers_frame[8] = new_stream_id & 0xFF;
	response_headers_frame[9] = 0x88; /* HPACK :status: 200 */
	response_headers_frame[10] = 0x5a;
	response_headers_frame[11] = 0x04;
	response_headers_frame[12] = 0x67;
	response_headers_frame[13] = 0x7a;
	response_headers_frame[14] = 0x69;
	response_headers_frame[15] = 0x70;
	/* HPACK content-encoding: gzip */
}

void send_data(int socket_fd, const char *payload, size_t length, uint8_t type,
	uint8_t flags, uint32_t stream_id)
{
	if (9 + length > MAX_FRAME_SIZE) {
		printf("Payload is too large for the buffer\n");
		return;
	}

	uint8_t data_frame[MAX_FRAME_SIZE];

	data_frame[0] = (length >> 16) & 0xFF;
	data_frame[1] = (length >> 8) & 0xFF;
	data_frame[2] = length & 0xFF;

	data_frame[3] = type;
	data_frame[4] = flags;

	data_frame[5] = (stream_id >> 24) & 0xFF;
	data_frame[6] = (stream_id >> 16) & 0xFF;
	data_frame[7] = (stream_id >> 8) & 0xFF;
	data_frame[8] = stream_id & 0xFF;

	memcpy(data_frame + 9, payload, length);

	int frame_size = 9 + length;

	if (sendall(socket_fd, (const char *)data_frame, frame_size) < 0) {
		LOG_ERR("ERROR writing to socket");
		return;
	}
}

const char *get_frame_type_name(enum http2_frame_type type)
{
	switch (type) {
	case HTTP2_DATA_FRAME:
		return "DATA";
	case HTTP2_HEADERS_FRAME:
		return "HEADERS";
	case HTTP2_PRIORITY_FRAME:
		return "PRIORITY";
	case HTTP2_RST_STREAM_FRAME:
		return "RST_STREAM";
	case HTTP2_SETTINGS_FRAME:
		return "SETTINGS";
	case HTTP2_PUSH_PROMISE_FRAME:
		return "PUSH_PROMISE";
	case HTTP2_PING_FRAME:
		return "PING";
	case HTTP2_GOAWAY_FRAME:
		return "GOAWAY";
	case HTTP2_WINDOW_UPDATE_FRAME:
		return "WINDOW_UPDATE";
	case HTTP2_CONTINUATION_FRAME:
		return "CONTINUATION";
	default:
		return "UNKNOWN";
	}
}

void print_http2_frames(struct http2_frame *frame)
{
	const char *bold = "\033[1m";
	const char *reset = "\033[0m";
	const char *green = "\033[32m";
	const char *blue = "\033[34m";

	printf("%s=====================================%s\n", green, reset);
	printf("%sReceived %s Frame :%s\n", bold,
		get_frame_type_name(frame->type), reset);
	printf("  %sLength:%s %u\n", blue, reset, frame->length);
	printf("  %sType:%s %u (%s)\n", blue, reset, frame->type,
		get_frame_type_name(frame->type));
	printf("  %sFlags:%s %u\n", blue, reset, frame->flags);
	printf("  %sStream Identifier:%s %u\n", blue, reset,
		frame->stream_identifier);
	printf("  %sPayload:%s ", blue, reset);
	for (unsigned int j = 0; j < frame->length; j++)
		printf("%02x ", frame->payload[j]);
	printf("\n");
	printf("%s=====================================%s\n\n\n", green, reset);
}

int parse_http2_frame(unsigned char *buffer, unsigned long buffer_len,
	struct http2_frame *frame)
{
	if (buffer_len < HTTP2_FRAME_HEADER_SIZE)
		return 0;

	frame->length = (buffer[HTTP2_FRAME_LENGTH_OFFSET] << 16) |
			(buffer[HTTP2_FRAME_LENGTH_OFFSET + 1] << 8) |
			buffer[HTTP2_FRAME_LENGTH_OFFSET + 2];
	frame->type = buffer[HTTP2_FRAME_TYPE_OFFSET];
	frame->flags = buffer[HTTP2_FRAME_FLAGS_OFFSET];
	frame->stream_identifier =
		(buffer[HTTP2_FRAME_STREAM_ID_OFFSET] << 24) |
		(buffer[HTTP2_FRAME_STREAM_ID_OFFSET + 1] << 16) |
		(buffer[HTTP2_FRAME_STREAM_ID_OFFSET + 2] << 8) |
		buffer[HTTP2_FRAME_STREAM_ID_OFFSET + 3];
	frame->stream_identifier &= 0x7FFFFFFF;

	if (buffer_len < HTTP2_FRAME_HEADER_SIZE + frame->length)
		return 0;

	frame->payload = buffer + HTTP2_FRAME_HEADER_SIZE;

	return 1;
}

bool settings_ack_flag(unsigned char flags)
{
	return (flags & HTTP2_FLAG_SETTINGS_ACK) != 0;
}

bool settings_end_headers_flag(unsigned char flags)
{
	return (flags & HTTP2_FLAG_END_HEADERS) != 0;
}

bool settings_stream_flag(unsigned char flags)
{
	return (flags & HTTP2_FLAG_STREAM) != 0;
}

unsigned int determine_frame_size(unsigned char *buffer)
{
	unsigned int length = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];

	return length + 9;
}

int find_headers_frame_stream_id(struct http2_frame *frame)
{
	if (frame->type == HTTP2_HEADERS_FRAME)
		return frame->stream_identifier;
	return -1;
}

enum http2_server_state determine_server_state(unsigned char *buffer)
{
	unsigned char frame_type = buffer[3];

	switch (frame_type) {
	case HTTP2_DATA_FRAME:
		return HTTP2_FRAME_DATA_STATE;
	case HTTP2_HEADERS_FRAME:
		return HTTP2_FRAME_HEADERS_STATE;
	case HTTP2_PRIORITY_FRAME:
		return HTTP2_FRAME_PRIORITY_STATE;
	case HTTP2_RST_STREAM_FRAME:
		return HTTP2_FRAME_RST_STREAM_STATE;
	case HTTP2_SETTINGS_FRAME:
		return HTTP2_FRAME_SETTINGS_STATE;
	case HTTP2_PING_FRAME:
		return HTTP2_FRAME_PING_STATE;
	case HTTP2_GOAWAY_FRAME:
		return HTTP2_FRAME_GOAWAY_STATE;
	case HTTP2_WINDOW_UPDATE_FRAME:
		return HTTP2_FRAME_WINDOW_UPDATE_STATE;
	case HTTP2_CONTINUATION_FRAME:
		return HTTP2_FRAME_CONTINUATION_STATE;
	default:
		return HTTP_DONE_STATE;
	}
}

enum http2_streaming_state determine_stream_state(unsigned char *buffer)
{
	unsigned char frame_type = buffer[3];

	switch (frame_type) {
	case HTTP2_HEADERS_FRAME:
		return IDLE_STATE;
	default:
		return OPEN_STATE;
	}
}
