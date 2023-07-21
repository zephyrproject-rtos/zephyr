/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "headers/config.h"
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

static char url_buffer[MAX_URL_LENGTH];
static char buffer[BUFFER_SIZE] = {0};
static char http_response[BUFFER_SIZE];
static unsigned char ubuffer[BUFFER_SIZE];
const char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

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

struct http2_frame frames[MAX_FRAMES];

static const char content[] = {
#include "index.html.gz.inc"
};

bool has_upgrade_header = true;

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

void close_client_connection(struct http2_server_ctx *ctx, int client_index)
{
	client_index += 2;
	close(ctx->client_fds[client_index].fd);
	ctx->client_fds[client_index].fd = 0;
	ctx->client_fds[client_index].events = 0;
	ctx->client_fds[client_index].revents = 0;

	if (client_index == ctx->num_clients) {
		while (ctx->num_clients > 0 &&
			ctx->client_fds[ctx->num_clients].fd == 0) {
			--ctx->num_clients;
		}
	}
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

void handle_http1_request(struct http2_server_ctx *ctx, int i, int *valread)
{
	const char *data;
	int len;
	int total_received = 0;
	int offset = 0;

	http_parser_init(&parser, HTTP_REQUEST);
	http_parser_settings_init(&parserSettings);
	parserSettings.on_header_field = on_header_field;
	http_parser_execute(
		&parser, &parserSettings, buffer + offset, *valread);

	total_received += *valread;
	offset += *valread;

	if (offset >= BUFFER_SIZE)
		offset = 0;

	if (has_upgrade_header == false) {
		handle_http2_request(ctx, i, valread);
	} else {
		data = content;
		len = sizeof(content);

		parserSettings.on_url = on_url;
		http_parser_execute(
			&parser, &parserSettings, buffer, BUFFER_SIZE);

		if (strcmp(url_buffer, "/") == 0) {
			sprintf(http_response,
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/html\r\n"
				"Content-Encoding: gzip\r\n"
				"Content-Length: %d\r\n\r\n",
				len);
			if (sendall(ctx->clients[i].client_fd, http_response,
				    strlen(http_response)) < 0) {
				close_client_connection(ctx, i);
			} else {

				if (sendall(ctx->clients[i].client_fd, data,
					    len) < 0) {
					LOG_ERR("sendall failed");
					close_client_connection(ctx, i);
				}
			}
		} else {
			const char *not_found_response =
				"HTTP/1.1 404 Not Found\r\n"
				"Content-Length: 9\r\n\r\n"
				"Not Found";
			if (sendall(ctx->clients[i].client_fd,
				    not_found_response,
				    strlen(not_found_response)) < 0) {
				close_client_connection(ctx, i);
			}
		}
		close_client_connection(ctx, i);
	}
}

void handle_awaiting_preface(struct http2_server_ctx *ctx, int i, int *valread)
{
	printf("AWAITING_PREFACE\n");
	const char *response = "HTTP/1.1 101 Switching Protocols\r\n"
			       "Connection: Upgrade\r\n"
			       "Upgrade: h2c\r\n"
			       "\r\n";
	if (sendall(ctx->clients[i].client_fd, response, strlen(response)) < 0)
		close_client_connection(ctx, i);

	/* Read the client data */
	*valread = recv(ctx->clients[i].client_fd, buffer, BUFFER_SIZE, 0);
	if (*valread < 0) {
		LOG_ERR("read failed");
		close_client_connection(ctx, i);
	} else if (*valread == 0) {
		LOG_INF("Connection closed by peer.\n");
		close_client_connection(ctx, i);
	} else {
		/* Check the client preface */
		if (strncmp(buffer, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n",
			    strlen(preface)) != 0) {
			LOG_INF("Client does not support HTTP/2.\n");
		} else {
			LOG_INF("The client support HTTP/2.\n");
		}
	}

	ctx->clients[i].state = READING_SETTINGS;
}

void handle_reading_settings(struct http2_server_ctx *ctx, int i, int *valread)
{
	printf("READING_SETTINGS\n");
	unsigned char frame[100];
	ssize_t readBytes = 0;
	unsigned int frame_count;

	ctx->clients[i].stream_id = 1;

	if (*valread > 24) {
		/* Define a new buffer for the remaining data and copy it there
		 */
		int preface_length = strlen(preface);
		int remaining_length = *valread - strlen(preface);

		memcpy(ubuffer, buffer + preface_length, remaining_length);

		frame_count =
			parse_http2_frames(ubuffer, remaining_length, frames);
		printf("frames count: %d\n", frame_count);
		print_http2_frames(frames, frame_count);
		ctx->clients[i].stream_id =
			find_headers_frame_stream_id(frames, frame_count);
		printf("stream_header_id: %d\n", ctx->clients[i].stream_id);

		ctx->clients[i].state = STREAMING;

	} else {
		readBytes = recv(
			ctx->clients[i].client_fd, frame, sizeof(frame), 0);
		if (readBytes < 0) {
			LOG_ERR("ERROR reading from socket");
			close_client_connection(ctx, i);
		} else if (readBytes == 0) {
			printf("Connection closed by peer.\n");
		}

		frame_count = parse_http2_frames(frame, readBytes, frames);
		print_http2_frames(frames, frame_count);

		readBytes = recv(
			ctx->clients[i].client_fd, frame, sizeof(frame), 0);
		if (readBytes < 0) {
			LOG_ERR("ERROR reading from socket");
			close_client_connection(ctx, i);
		} else if (readBytes == 0) {
			printf("Connection closed by peer.\n");
		}
		frame_count = parse_http2_frames(frame, readBytes, frames);
		print_http2_frames(frames, frame_count);
	}
	/* Send a SETTINGS frame after receiving a valid preface */
	ssize_t ret = sendall(ctx->clients[i].client_fd, settings_frame,
		sizeof(settings_frame));
	if (ret < 0) {
		LOG_ERR("ERROR writing to socket");
		return;
	}

	ret = sendall(
		ctx->clients[i].client_fd, settings_ack, sizeof(settings_ack));
	if (ret < 0) {
		LOG_ERR("ERROR writing to socket");
		return;
	}

	ctx->clients[i].state = STREAMING;
}

void handle_streaming(struct http2_server_ctx *ctx, int i)
{
	printf("STREAMING\n");
	unsigned char frame[100];
	ssize_t readBytes = 0;
	unsigned int frame_count;

	/* Read the header of the next frame */
	readBytes = recv(ctx->clients[i].client_fd, frame, sizeof(frame), 0);
	if (readBytes < 0) {
		LOG_ERR("ERROR reading from socket");
		close_client_connection(ctx, i);
	} else if (readBytes == 0) {
		LOG_INF("Connection closed by peer.\n");
	}
	frame_count = parse_http2_frames(frame, readBytes, frames);
	print_http2_frames(frames, frame_count);

	ssize_t ret = sendall(ctx->clients[i].client_fd, settings_frame,
		sizeof(settings_frame));
	if (ret < 0) {
		LOG_ERR("ERROR writing to socket");
		return;
	}

	unsigned char response_headers_frame[16];

	generate_response_headers_frame(
		response_headers_frame, ctx->clients[i].stream_id);

	ret = sendall(ctx->clients[i].client_fd, response_headers_frame,
		sizeof(response_headers_frame));
	if (ret < 0) {
		LOG_ERR("ERROR writing to socket");
		return;
	}

	readBytes = recv(ctx->clients[i].client_fd, frame, sizeof(frame), 0);
	if (readBytes < 0) {
		LOG_ERR("ERROR reading from socket");
		close_client_connection(ctx, i);
	} else if (readBytes == 0) {
		LOG_INF("Connection closed by peer.\n");
	}

	frame_count = parse_http2_frames(frame, readBytes, frames);
	print_http2_frames(frames, frame_count);

	size_t content_size = sizeof(content);

	send_data(ctx->clients[i].client_fd, content, content_size, 0x00, 0x01,
		0x01);

	readBytes = recv(ctx->clients[i].client_fd, frame, sizeof(frame), 0);
	if (readBytes < 0) {
		LOG_ERR("ERROR reading from socket");
		close_client_connection(ctx, i);
	} else if (readBytes == 0) {
		LOG_INF("Connection closed by peer.\n");
	}

	frame_count = parse_http2_frames(frame, readBytes, frames);
	print_http2_frames(frames, frame_count);

	ctx->clients[i].state = CLOSING;

	has_upgrade_header = true;
	close_client_connection(ctx, i);
}

void handle_closing(struct http2_server_ctx *ctx, int i)
{
	printf("CLOSING\n");
	close_client_connection(ctx, i);
}

void handle_http2_request(struct http2_server_ctx *ctx, int i, int *valread)
{
	printf("Hello HTTP/2.\n");

	/* Initialize state */
	if (has_upgrade_header == false)
		ctx->clients[i].state = AWAITING_PREFACE;
	else
		ctx->clients[i].state = READING_SETTINGS;

	/* Handle different states */
	while (1) {
		switch (ctx->clients[i].state) {
		case AWAITING_PREFACE:
			handle_awaiting_preface(ctx, i, valread);
			break;
		case READING_SETTINGS:
			handle_reading_settings(ctx, i, valread);
			break;
		case STREAMING:
			handle_streaming(ctx, i);
			break;
		case CLOSING:
			handle_closing(ctx, i);
			break;
		default:
			LOG_ERR("Unknown state.\n");
			close_client_connection(ctx, i);
		}

		if (ctx->clients[i].state == CLOSING)
			break;
	}
}

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

	/* Initialize client_fds */
	memset(ctx->client_fds, 0, sizeof(ctx->client_fds));
	memset(ctx->clients, 0, sizeof(ctx->clients));

	ctx->client_fds[0].fd = ctx->server_fd;
	ctx->client_fds[0].events = POLLIN;

	ctx->client_fds[1].fd = ctx->event_fd;
	ctx->client_fds[1].events = POLLIN;

	ctx->num_clients = 0;
	ctx->infinite = 1;

	return ctx->server_fd;
}

int http2_server_start(struct http2_server_ctx *ctx)
{
	printf("\nType 'quit' to quit\n\n");
	printf("Waiting for incoming connections...\n");

	eventfd_t value;

	do {
		int ret = poll(ctx->client_fds, ctx->num_clients + 2, 10000);

		if (ret < 0) {
			LOG_ERR("poll failed");
			return -errno;
		}

		for (int i = 0; i <= ctx->num_clients + 1; i++) {
			if (ctx->client_fds[i].revents & POLLERR) {
				LOG_ERR("Error on fd %d\n",
					ctx->client_fds[i].fd);
				close_client_connection(ctx, i);
				continue;
			}

			if (ctx->client_fds[i].revents & POLLHUP) {
				LOG_INF("Client on fd %d has disconnected\n",
					ctx->client_fds[i].fd);
				close_client_connection(ctx, i);
				continue;
			}

			if (!(ctx->client_fds[i].revents & POLLIN))
				continue;

			if (i == 0) {
				int new_socket =
					accept_new_client(ctx->server_fd);

				for (int j = 2; j < MAX_CLIENTS + 2; j++) {
					if (ctx->client_fds[j].fd != 0)
						continue;

					ctx->client_fds[j].fd = new_socket;
					ctx->client_fds[j].events = POLLIN;
					ctx->clients[j - 2].client_fd =
						new_socket;

					if (j > ctx->num_clients + 1)
						ctx->num_clients++;

					break;
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

			int valread = recv(ctx->clients[i - 2].client_fd,
				buffer, BUFFER_SIZE, 0);

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

			/* Check the client preface */
			if (strncmp(buffer, preface, strlen(preface)) != 0) {
				LOG_INF("Client does not support HTTP/2.\n");
				handle_http1_request(ctx, i - 2, &valread);
				continue;
			}

			handle_http2_request(ctx, i - 2, &valread);
		}
	} while (ctx->infinite == 1);
	return 0;
}

void generate_response_headers_frame(
	unsigned char *response_headers_frame, int new_stream_id)
{
	response_headers_frame[0] = 0x00;
	response_headers_frame[1] = 0x00;
	response_headers_frame[2] = 0x07;
	response_headers_frame[3] = 0x01;
	response_headers_frame[4] = 0x04;
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

void print_http2_frames(struct http2_frame *frames, unsigned int frame_count)
{
	for (unsigned int i = 0; i < frame_count; i++) {
		printf("Frame %d:\n", i);
		printf("  Length: %u\n", frames[i].length);
		printf("  Type: %u\n", frames[i].type);
		printf("  Flags: %u\n", frames[i].flags);
		printf("  Stream Identifier: %u\n",
			frames[i].stream_identifier);
		printf("  Payload: ");
		for (unsigned int j = 0; j < frames[i].length; j++)
			printf("%02x ", frames[i].payload[j]);

		printf("\n\n");
	}
}

unsigned int parse_http2_frames(unsigned char *buffer, unsigned long buffer_len,
	struct http2_frame *frames)
{
	unsigned int frame_count = 0;
	unsigned long pos = 0;

	while (pos + 9 <= buffer_len && frame_count < MAX_FRAMES) {
		frames[frame_count].length = (buffer[pos] << 16) |
					     (buffer[pos + 1] << 8) |
					     buffer[pos + 2];
		frames[frame_count].type = buffer[pos + 3];
		frames[frame_count].flags = buffer[pos + 4];
		frames[frame_count].stream_identifier =
			(buffer[pos + 5] << 24) | (buffer[pos + 6] << 16) |
			(buffer[pos + 7] << 8) | buffer[pos + 8];
		frames[frame_count].stream_identifier &= 0x7FFFFFFF;

		pos += 9;

		/* Ckeck if payload size doesn't exceed buffer size */
		if (pos + frames[frame_count].length > buffer_len)
			break;

		frames[frame_count].payload = buffer + pos;

		pos += frames[frame_count].length;

		frame_count++;
	}

	return frame_count;
}

int find_headers_frame_stream_id(
	struct http2_frame *frames, unsigned int frame_count)
{
	for (unsigned int i = 0; i < frame_count; i++) {
		if (frames[i].type == FRAME_TYPE_HEADERS)
			return frames[i].stream_identifier;
	}
	return -1;
}
