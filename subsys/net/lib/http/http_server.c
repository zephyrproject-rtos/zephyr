/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "headers/server_functions.h"
#include "headers/http_service.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifdef __linux__

#define CONFIG_NET_HTTP_SERVER_MAX_RESPONSE_SIZE 128
#define CONFIG_NET_HTTP_SERVER_MAX_URL_LENGTH    64
#define CONFIG_NET_HTTP_SERVER_MAX_FRAME_SIZE    2048

#endif

#define HTTP_SERVER_MAX_RESPONSE_SIZE CONFIG_NET_HTTP_SERVER_MAX_RESPONSE_SIZE
#define HTTP_SERVER_MAX_URL_LENGTH    CONFIG_NET_HTTP_SERVER_MAX_URL_LENGTH
#define HTTP_SERVER_MAX_FRAME_SIZE    CONFIG_NET_HTTP_SERVER_MAX_FRAME_SIZE

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)

#define INCLUDE_HTML_CONTENT 1

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
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/sys/eventfd.h>

#endif

#if defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)

#define SERVER_IPV4_ADDR "192.0.2.1"
#include <zephyr/data/json.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_http_server, LOG_LEVEL_DBG);
#include <zephyr/net/tls_credentials.h>

#elif defined(__linux__)

#define SERVER_IPV4_ADDR "127.0.0.1"
#include "headers/mlog.h"
#include <jansson.h>

#endif

static char url_buffer[CONFIG_NET_HTTP_SERVER_MAX_URL_LENGTH];
static char http_response[CONFIG_NET_HTTP_SERVER_MAX_RESPONSE_SIZE];
static const char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

static struct http_parser_settings parserSettings;
static struct http_parser parser;

static unsigned char settings_frame[9] = {
	0x00, 0x00, 0x00,        /* Length */
	0x04,                    /* Type: 0x04 - setting frames for config or acknowledgment */
	0x00,                    /* Flags: 0x00 - unused flags */
	0x00, 0x00, 0x00, 0x00}; /* Reserved, Stream Identifier: 0x00 - overall connection */

static unsigned char settings_ack[9] = {
	0x00, 0x00, 0x00,        /* Length */
	0x04,                    /* Type: 0x04 - setting frames for config or acknowledgment */
	0x01,                    /* Flags: 0x01 - ACK */
	0x00, 0x00, 0x00, 0x00}; /* Reserved, Stream Identifier */

static const char content_200[] = {
#ifdef INCLUDE_HTML_CONTENT
#include "index.html.gz.inc"
#endif
};

static const char content_404[] = {
#ifdef INCLUDE_HTML_CONTENT
#include "not_found_page.html.gz.inc"
#endif
};

bool has_upgrade_header = true;

#if defined(__ZEPHYR__)
static const struct json_obj_descr arithmetic_result_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct arithmetic_result, x, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct arithmetic_result, y, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct arithmetic_result, result, JSON_TOK_NUMBER),
};
#endif

int http_server_init(struct http_server_ctx *ctx)
{
	int proto = IPPROTO_TCP;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	proto = IPPROTO_TLS_1_2;
#endif

	/* Create a socket */
#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)
	ctx->server_fd = socket(ctx->config.address_family, SOCK_STREAM, proto);
#else
	ctx->server_fd = zsock_socket(ctx->config.address_family, SOCK_STREAM, proto);
#endif
	if (ctx->server_fd < 0) {
		LOG_ERR("socket");
		return ctx->server_fd;
	}

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)
	if (setsockopt(ctx->server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
		LOG_ERR("setsockopt");
		return -errno;
	}
#else
	if (zsock_setsockopt(ctx->server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) <
	    0) {
		LOG_ERR("setsockopt");
		return -errno;
	}
#endif

#if defined(CONFIG_TLS_CREDENTIALS) || defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	static const sec_tag_t server_tag_list_verify_none[] = {
		HTTP_SERVER_SERVER_CERTIFICATE_TAG,
	};

	const sec_tag_t *sec_tag_list;
	size_t sec_tag_list_size;

	sec_tag_list = server_tag_list_verify_none;
	sec_tag_list_size = sizeof(server_tag_list_verify_none);

	if (setsockopt(ctx->server_fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list, sec_tag_list_size) <
	    0) {
		LOG_ERR("setsockopt");
		return -errno;
	}

	if (setsockopt(ctx->server_fd, SOL_TLS, TLS_HOSTNAME, "localhost", sizeof("localhost")) <
	    0) {
		LOG_ERR("setsockopt");
		return -errno;
	}
#endif

	/* Set up the server address struct according to address family */
	if (ctx->config.address_family == AF_INET) {
		struct sockaddr_in serv_addr;

		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(ctx->config.port);

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)
		if (bind(ctx->server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			LOG_ERR("bind");
			return -errno;
		}
#else
		if (zsock_bind(ctx->server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <
		    0) {
			LOG_ERR("bind");
			return -errno;
		}
#endif

	} else if (ctx->config.address_family == AF_INET6) {
		struct sockaddr_in6 serv_addr;

		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin6_family = AF_INET6;
		serv_addr.sin6_addr = in6addr_any;
		serv_addr.sin6_port = htons(ctx->config.port);

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)
		if (bind(ctx->server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			LOG_ERR("bind");
			return -errno;
		}
#else
		if (zsock_bind(ctx->server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <
		    0) {
			LOG_ERR("bind");
			return -errno;
		}
#endif
	}

	/* Listen for connections */
#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)
	if (listen(ctx->server_fd, MAX_CLIENTS) < 0) {
		LOG_ERR("listen");
		return -errno;
	}
#else
	if (zsock_listen(ctx->server_fd, MAX_CLIENTS) < 0) {
		LOG_ERR("listen");
		return -errno;
	}
#endif

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
	ctx->results_count = 0;
	ctx->infinite = 1;

	return ctx->server_fd;
}

int accept_new_client(int server_fd)
{
	int new_socket;
	socklen_t addrlen;
	struct sockaddr_storage sa;

	memset(&sa, 0, sizeof(sa));
	addrlen = sizeof(sa);

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)
	new_socket = accept(server_fd, (struct sockaddr *)&sa, &addrlen);
#else
	new_socket = zsock_accept(server_fd, (struct sockaddr *)&sa, &addrlen);
#endif
	if (new_socket < 0) {
		LOG_ERR("accept failed");
		return new_socket;
	}

	return new_socket;
}

int http_server_start(struct http_server_ctx *ctx)
{
	LOG_INF("Waiting for incoming connections at http://%s:%d", SERVER_IPV4_ADDR,
		ctx->config.port);

	eventfd_t value;

	do {
		int ret = poll(ctx->fds, ctx->num_clients + 2, 0);

		if (ret < 0) {
			LOG_ERR("poll failed");
			return -errno;
		}

		for (int i = 0; i < ctx->num_clients + 2; i++) {
			struct http_client_ctx *client = &ctx->clients[i - 2];

			if (ctx->fds[i].revents & POLLERR) {
				LOG_ERR("Error on fd %d", ctx->fds[i].fd);
				close_client_connection(ctx, i);
				continue;
			}

			if (ctx->fds[i].revents & POLLHUP) {
				LOG_INF("Client on fd %d has disconnected", ctx->fds[i].fd);
				close_client_connection(ctx, i);
				continue;
			}

			if (!(ctx->fds[i].revents & POLLIN)) {
				continue;
			}

			if (i == 0) {
				int new_socket = accept_new_client(ctx->server_fd);

				bool found_slot = false;

				for (int j = 2; j < MAX_CLIENTS + 1; j++) {
					if (ctx->fds[j].fd != 0) {
						continue;
					}

					ctx->fds[j].fd = new_socket;
					ctx->fds[j].events = POLLIN;

					initialize_client_ctx(&ctx->clients[j - 2], new_socket);

					if (j > ctx->num_clients) {
						ctx->num_clients++;
					}

					found_slot = true;
					break;
				}
				if (!found_slot) {
					LOG_INF("No free slot found.");
#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)
					close(new_socket);
#else
					zsock_close(new_socket);
#endif
				}
				continue;
			}

			if (i == 1) {
#ifdef __ZEPHYR__
				eventfd_read(ctx->event_fd, &value);
#else
				read(ctx->event_fd, &value, sizeof(value));
#endif
				LOG_DBG("Received stop event. exiting ..");

				return 0;
			}

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)
			int valread = recv(client->client_fd, client->buffer + client->offset,
					   sizeof(client->buffer) - client->offset, 0);
#else
			int valread = zsock_recv(client->client_fd, client->buffer + client->offset,
						 sizeof(client->buffer) - client->offset, 0);
#endif
			if (valread <= 0) {
				if (valread == 0) {
					LOG_INF("Connection closed by peer");
				} else {
					LOG_ERR("ERROR reading from socket");
				}

				close_client_connection(ctx, i);
				continue;
			}

			client->offset += valread;
			handle_http_request(ctx, client, i);
		}
	} while (ctx->infinite == 1);
	return 0;
}

void close_client_connection(struct http_server_ctx *ctx_server, int client_index)
{
#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)
	close(ctx_server->fds[client_index].fd);
#else
	zsock_close(ctx_server->fds[client_index].fd);
#endif
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

void initialize_client_ctx(struct http_client_ctx *ctx_client, int new_socket)
{
	ctx_client->client_fd = new_socket;
	ctx_client->offset = 0;
	ctx_client->server_state = HTTP_SERVER_PREFACE_STATE;
	for (int i = 0; i < MAX_STREAMS; i++) {
		ctx_client->streams[i].stream_state = HTTP_SERVER_STREAM_IDLE;
		ctx_client->streams[i].stream_id = 0;
	}
}

struct http_stream_ctx *find_http_stream_context(struct http_client_ctx *ctx_client,
						 uint32_t stream_id)
{
	for (int i = 0; i < MAX_STREAMS; i++) {
		if (ctx_client->streams[i].stream_id == stream_id) {
			return &ctx_client->streams[i];
		}
	}
	return NULL;
}

struct http_stream_ctx *allocate_http_stream_context(struct http_client_ctx *ctx_client,
						     uint32_t stream_id)
{
	for (int i = 0; i < MAX_STREAMS; i++) {
		if (ctx_client->streams[i].stream_state == HTTP_SERVER_STREAM_IDLE) {
			ctx_client->streams[i].stream_id = stream_id;
			ctx_client->streams[i].stream_state = HTTP_SERVER_STREAM_OPEN;
			return &ctx_client->streams[i];
		}
	}
	return NULL;
}

int handle_http_request(struct http_server_ctx *ctx_server, struct http_client_ctx *ctx_client,
			int client_index)
{
	int ret = -EINVAL;

	do {
		switch (ctx_client->server_state) {
		case HTTP_SERVER_PREFACE_STATE:
			ret = handle_http_preface(ctx_client);
			break;
		case HTTP_SERVER_REQUEST_STATE:
			ret = handle_http1_request(ctx_server, ctx_client, client_index);
			break;
		case HTTP_SERVER_FRAME_HEADER_STATE:
			ret = handle_http_frame_header(ctx_server, ctx_client, client_index);
			break;
		case HTTP_SERVER_FRAME_HEADERS_STATE:
			ret = handle_http_frame_headers(ctx_client);
			break;
		case HTTP_SERVER_FRAME_CONTINUATION_STATE:
			ret = handle_http_frame_continuation(ctx_client);
			break;
		case HTTP_SERVER_FRAME_SETTINGS_STATE:
			ret = handle_http_frame_settings(ctx_client);
			break;
		case HTTP_SERVER_FRAME_WINDOW_UPDATE_STATE:
			ret = handle_http_frame_window_update(ctx_client);
			break;
		case HTTP_SERVER_FRAME_RST_STREAM_STATE:
			ret = handle_http_frame_rst_frame(ctx_server, ctx_client, client_index);
			break;
		case HTTP_SERVER_FRAME_GOAWAY_STATE:
			ret = handle_http_frame_goaway(ctx_server, ctx_client, client_index);
			break;
		case HTTP_SERVER_FRAME_PRIORITY_STATE:
			ret = handle_http_frame_priority(ctx_client);
			break;
		case HTTP_SERVER_DONE_STATE:
			ret = handle_http_done(ctx_server, ctx_client, client_index);
			break;
		default:
			ret = handle_http_done(ctx_server, ctx_client, client_index);
			break;
		}

	} while (ret == 0 && ctx_client->offset > 0);

	return ret;
}

int handle_http_frame_header(struct http_server_ctx *ctx_server, struct http_client_ctx *ctx_client,
			     int client_index)
{
	LOG_DBG("HTTP_SERVER_FRAME_HEADER");

	int parse_result = parse_http_frame_header(ctx_client);

	if (parse_result == 0) {
		return -EAGAIN;
	} else if (parse_result < 0) {
		return parse_result;
	}

	int bytes_consumed = HTTP_SERVER_FRAME_HEADER_SIZE;

	ctx_client->offset -= bytes_consumed;
	memmove(ctx_client->buffer, ctx_client->buffer + bytes_consumed, ctx_client->offset);

	switch (ctx_client->current_frame.type) {
	case HTTP_SERVER_HEADERS_FRAME:
		return enter_http_frame_headers_state(ctx_server, ctx_client, client_index);
	case HTTP_SERVER_CONTINUATION_FRAME:
		return enter_http_frame_continuation_state(ctx_client);
	case HTTP_SERVER_SETTINGS_FRAME:
		return enter_http_frame_settings_state(ctx_client);
	case HTTP_SERVER_WINDOW_UPDATE_FRAME:
		return enter_http_frame_window_update_state(ctx_client);
	case HTTP_SERVER_RST_STREAM_FRAME:
		return enter_http_frame_rst_stream_state(ctx_server, ctx_client, client_index);
	case HTTP_SERVER_GOAWAY_FRAME:
		return enter_http_frame_goaway_state(ctx_server, ctx_client, client_index);
	case HTTP_SERVER_PRIORITY_FRAME:
		return enter_http_frame_priority_state(ctx_client);
	default:
		return enter_http_http_done_state(ctx_server, ctx_client, client_index);
	}

	return 0;
}

int enter_http_frame_settings_state(struct http_client_ctx *ctx_client)
{
	ctx_client->server_state = HTTP_SERVER_FRAME_SETTINGS_STATE;

	return 0;
}

int enter_http_frame_headers_state(struct http_server_ctx *ctx_server,
				   struct http_client_ctx *ctx_client, int client_index)
{
	struct http_frame *frame = &ctx_client->current_frame;
	struct http_stream_ctx *stream =
		find_http_stream_context(ctx_client, frame->stream_identifier);

	if (!stream) {
		LOG_DBG("|| stream ID ||  %d", frame->stream_identifier);
		stream = allocate_http_stream_context(ctx_client, frame->stream_identifier);

		if (!stream) {
			LOG_ERR("No available stream slots. Connection closed.");
			close_client_connection(ctx_server, client_index);

			return -ENOMEM;
		}
	}

	if (settings_end_headers_flag(ctx_client->current_frame.flags) &&
	    settings_end_stream_flag(ctx_client->current_frame.flags)) {
		ctx_client->server_state = HTTP_SERVER_FRAME_HEADERS_STATE;
	} else {
		ctx_client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;
	}

	return 0;
}

int enter_http_frame_continuation_state(struct http_client_ctx *ctx_client)
{
	ctx_client->server_state = HTTP_SERVER_FRAME_CONTINUATION_STATE;

	return 0;
}

int enter_http_frame_window_update_state(struct http_client_ctx *ctx_client)
{
	ctx_client->server_state = HTTP_SERVER_FRAME_WINDOW_UPDATE_STATE;

	return 0;
}

int enter_http_frame_priority_state(struct http_client_ctx *ctx_client)
{
	ctx_client->server_state = HTTP_SERVER_FRAME_PRIORITY_STATE;

	return 0;
}

int enter_http_frame_rst_stream_state(struct http_server_ctx *ctx_server,
				      struct http_client_ctx *ctx_client, int client_index)
{
	ctx_client->server_state = HTTP_SERVER_FRAME_RST_STREAM_STATE;

	return 0;
}

int enter_http_frame_goaway_state(struct http_server_ctx *ctx_server,
				  struct http_client_ctx *ctx_client, int client_index)
{
	ctx_client->server_state = HTTP_SERVER_FRAME_GOAWAY_STATE;

	return 0;
}

int enter_http_http_done_state(struct http_server_ctx *ctx_server,
			       struct http_client_ctx *ctx_client, int client_index)
{
	ctx_client->server_state = HTTP_SERVER_DONE_STATE;

	return 0;
}

int handle_http_preface(struct http_client_ctx *ctx_client)
{
	LOG_DBG("HTTP_SERVER_PREFACE_STATE.");
	if (ctx_client->offset < strlen(preface)) {
		/* We don't have full preface yet, get more data. */
		return -EAGAIN;
	}

	if (strncmp(ctx_client->buffer, preface, strlen(preface)) != 0) {
		ctx_client->server_state = HTTP_SERVER_REQUEST_STATE;
	} else {
		ctx_client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

		ctx_client->offset -= strlen(preface);
		memmove(ctx_client->buffer, ctx_client->buffer + strlen(preface),
			ctx_client->offset);
	}

	return 0;
}

int handle_http1_request(struct http_server_ctx *ctx_server, struct http_client_ctx *ctx_client,
			 int client_index)
{
	LOG_DBG("HTTP_SERVER_REQUEST.");
	const char *data;
	int len;
	int total_received = 0;
	int offset = 0;

	http_parser_init(&parser, HTTP_REQUEST);
	http_parser_settings_init(&parserSettings);
	parserSettings.on_header_field = on_header_field;
	parserSettings.on_url = on_url;
	http_parser_execute(&parser, &parserSettings, ctx_client->buffer + offset,
			    ctx_client->offset);

	total_received += ctx_client->offset;
	offset += ctx_client->offset;

	if (offset >= HTTP_SERVER_MAX_RESPONSE_SIZE) {
		offset = 0;
	}

	if (has_upgrade_header == false) {
		const char *response = "HTTP/1.1 101 Switching Protocols\r\n"
				       "Connection: Upgrade\r\n"
				       "Upgrade: h2c\r\n"
				       "\r\n";
		if (sendall(ctx_client->client_fd, response, strlen(response)) < 0) {
			close_client_connection(ctx_server, client_index);
		}

		memset(ctx_client->buffer, 0, sizeof(ctx_client->buffer));
		ctx_client->offset = 0;
		ctx_client->server_state = HTTP_SERVER_PREFACE_STATE;

	} else {
		data = content_200;
		len = sizeof(content_200);

		enum http_method method = parser.method;
		const char *method_str = http_method_str(method);

		LOG_DBG("HTTP Method: %s", method_str);

		if (strncmp(method_str, "GET", 3) == 0 && strncmp(url_buffer, "/results", 8) == 0) {
			handle_get_request(ctx_server, ctx_client->client_fd);
		} else if (strncmp(method_str, "POST", 4) == 0 &&
			   strncmp(url_buffer, "/add", 4) == 0) {
			char *json_start = strstr(ctx_client->buffer, "\r\n\r\n");

			if (json_start) {
				handle_post_request(ctx_server, json_start);
			}

			char response[] = "HTTP/1.1 200 OK\r\nContent-Type: "
					  "text/plain\r\n\r\nAdded successfully.\n";
			sendall(ctx_client->client_fd, response, strlen(response));
		} else if (strncmp(method_str, "GET", 3) == 0 && strncmp(url_buffer, "/", 1) == 0) {
			sprintf(http_response,
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/html\r\n"
				"Content-Encoding: gzip\r\n"
				"Content-Length: %d\r\n\r\n",
				len);
			if (sendall(ctx_client->client_fd, http_response, strlen(http_response)) <
			    0) {
				close_client_connection(ctx_server, client_index);
			} else {

				if (sendall(ctx_client->client_fd, data, len) < 0) {
					LOG_ERR("sendall failed");
					close_client_connection(ctx_server, client_index);
				}
			}
		} else {
			const char *not_found_response = "HTTP/1.1 404 Not Found\r\n"
							 "Content-Length: 9\r\n\r\n"
							 "Not Found";
			if (sendall(ctx_client->client_fd, not_found_response,
				    strlen(not_found_response)) < 0) {
				close_client_connection(ctx_server, client_index);
			}
		}
		close_client_connection(ctx_server, client_index);
		memset(ctx_client->buffer, 0, sizeof(ctx_client->buffer));
		ctx_client->offset = 0;
	}

	return 0;
}

int handle_http_done(struct http_server_ctx *ctx_server, struct http_client_ctx *ctx_client,
		     int client_index)
{
	LOG_DBG("HTTP_SERVER_DONE_STATE");

	close_client_connection(ctx_server, client_index);

	return -errno;
}

int handle_http_frame_headers(struct http_client_ctx *ctx_client)
{
	LOG_DBG("HTTP_SERVER_FRAME_HEADERS");

	unsigned char response_headers_frame[16];

	struct http_frame *frame = &ctx_client->current_frame;

	print_http_frames(ctx_client);

	if (ctx_client->offset < frame->length) {
		return -EAGAIN;
	}

	const char *method;
	const char *path;

	if (!has_upgrade_header) {
		enum http_method method_h2c = parser.method;

		method = http_method_str(method_h2c);
		path = url_buffer;
	} else {
		method = http_hpack_parse_header(ctx_client, HTTP_SERVER_HPACK_METHOD);
		path = http_hpack_parse_header(ctx_client, HTTP_SERVER_HPACK_PATH);
	}

	if (strcmp(method, "GET") == 0 && strcmp(path, "/") == 0) {

		generate_response_headers_frame(response_headers_frame, frame->stream_identifier,
						HTTP_SERVER_HPACK_STATUS_2OO);

		size_t ret = sendall(ctx_client->client_fd, response_headers_frame,
				     sizeof(response_headers_frame));
		if (ret < 0) {
			LOG_ERR("ERROR writing to socket");
			return -errno;
		}

		size_t content_size = sizeof(content_200);

		send_data(ctx_client->client_fd, content_200, content_size, HTTP_SERVER_DATA_FRAME,
			  HTTP_SERVER_FLAG_END_STREAM, frame->stream_identifier);
	} else {
		generate_response_headers_frame(response_headers_frame, frame->stream_identifier,
						HTTP_SERVER_HPACK_STATUS_4O4);

		size_t ret = sendall(ctx_client->client_fd, response_headers_frame,
				     sizeof(response_headers_frame));
		if (ret < 0) {
			LOG_ERR("ERROR writing to socket");
			return -errno;
		}

		size_t content_size = sizeof(content_404);

		send_data(ctx_client->client_fd, content_404, content_size, HTTP_SERVER_DATA_FRAME,
			  HTTP_SERVER_FLAG_END_STREAM, frame->stream_identifier);
	}

	ctx_client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	int bytes_consumed = ctx_client->current_frame.length;

	ctx_client->offset -= bytes_consumed;
	memmove(ctx_client->buffer, ctx_client->buffer + bytes_consumed, ctx_client->offset);

	return 0;
}

int handle_http_frame_priority(struct http_client_ctx *ctx_client)
{
	LOG_DBG("HTTP_SERVER_FRAME_PRIORITY_STATE");

	struct http_frame *frame = &ctx_client->current_frame;

	print_http_frames(ctx_client);

	if (ctx_client->offset < frame->length) {
		return -EAGAIN;
	}

	int bytes_consumed = ctx_client->current_frame.length;

	ctx_client->offset -= bytes_consumed;
	memmove(ctx_client->buffer, ctx_client->buffer + bytes_consumed, ctx_client->offset);

	ctx_client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	return 0;
}

int handle_http_frame_continuation(struct http_client_ctx *ctx_client)
{
	LOG_DBG("HTTP_SERVER_FRAME_CONTINUATION_STATE");
	ctx_client->server_state = HTTP_SERVER_FRAME_HEADERS_STATE;

	return 0;
}

int handle_http_frame_settings(struct http_client_ctx *ctx_client)
{
	LOG_DBG("HTTP_SERVER_FRAME_SETTINGS");

	struct http_frame *frame = &ctx_client->current_frame;

	print_http_frames(ctx_client);

	if (ctx_client->offset < frame->length) {
		return -EAGAIN;
	}

	int bytes_consumed = ctx_client->current_frame.length;

	ctx_client->offset -= bytes_consumed;
	memmove(ctx_client->buffer, ctx_client->buffer + bytes_consumed, ctx_client->offset);

	if (!settings_ack_flag(frame->flags)) {
		ssize_t ret =
			sendall(ctx_client->client_fd, settings_frame, sizeof(settings_frame));
		if (ret < 0) {
			LOG_ERR("ERROR writing to socket");
			return -errno;
		}

		ret = sendall(ctx_client->client_fd, settings_ack, sizeof(settings_ack));
		if (ret < 0) {
			LOG_ERR("ERROR writing to socket");
			return -errno;
		}
	}

	ctx_client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	return 0;
}

int handle_http_frame_window_update(struct http_client_ctx *ctx_client)
{
	LOG_DBG("HTTP_SERVER_FRAME_WINDOW_UPDATE");

	struct http_frame *frame = &ctx_client->current_frame;

	print_http_frames(ctx_client);

	if (!has_upgrade_header) {

		frame->stream_identifier = 1;
		handle_http_frame_headers(ctx_client);
		ctx_client->server_state = HTTP_SERVER_FRAME_GOAWAY_STATE;

		return 0;
	}

	if (ctx_client->offset < frame->length) {
		return -EAGAIN;
	}

	int bytes_consumed = ctx_client->current_frame.length;

	ctx_client->offset -= bytes_consumed;
	memmove(ctx_client->buffer, ctx_client->buffer + bytes_consumed, ctx_client->offset);

	ctx_client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	return 0;
}

int handle_http_frame_goaway(struct http_server_ctx *ctx_server, struct http_client_ctx *ctx_client,
			     int client_index)
{
	LOG_DBG("HTTP_SERVER_FRAME_GOAWAY");

	struct http_frame *frame = &ctx_client->current_frame;

	print_http_frames(ctx_client);

	if (ctx_client->offset < frame->length) {
		return -EAGAIN;
	}

	int bytes_consumed = ctx_client->current_frame.length;

	ctx_client->offset -= bytes_consumed;
	memmove(ctx_client->buffer, ctx_client->buffer + bytes_consumed, ctx_client->offset);

	close_client_connection(ctx_server, client_index);
	has_upgrade_header = true;
	memset(ctx_client->buffer, 0, sizeof(ctx_client->buffer));

	ctx_client->offset = 0;

	return 0;
}

int handle_http_frame_rst_frame(struct http_server_ctx *ctx_server,
				struct http_client_ctx *ctx_client, int client_index)
{
	LOG_DBG("FRAME_RST_STREAM");

	struct http_frame *frame = &ctx_client->current_frame;

	print_http_frames(ctx_client);

	if (ctx_client->offset < frame->length) {
		return -EAGAIN;
	}

	int bytes_consumed = ctx_client->current_frame.length;

	ctx_client->offset -= bytes_consumed;
	memmove(ctx_client->buffer, ctx_client->buffer + bytes_consumed, ctx_client->offset);

	ctx_client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	return 0;
}

int on_header_field(struct http_parser *p, const char *at, size_t length)
{
	if (length == 7 && strncasecmp(at, "Upgrade", length) == 0) {
		LOG_INF("The \"Upgrade: h2c\" header is present.");
		has_upgrade_header = false;
	}
	return 0;
}

int on_url(struct http_parser *p, const char *at, size_t length)
{
	strncpy(url_buffer, at, length);
	url_buffer[length] = '\0';
	LOG_DBG("Requested URL: %s", url_buffer);
	return 0;
}

ssize_t sendall(int sock, const void *buf, size_t len)
{
	while (len) {
#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)
		size_t out_len = send(sock, buf, len, 0);
#else
		ssize_t out_len = zsock_send(sock, buf, len, 0);
#endif
		if (out_len < 0) {
			return out_len;
		}

		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

void generate_response_headers_frame(unsigned char *response_headers_frame, int new_stream_id,
				     int hpack_status)
{
	response_headers_frame[0] = 0x00;
	response_headers_frame[1] = 0x00;
	response_headers_frame[2] = 0x07;
	response_headers_frame[3] = 0x01;
	response_headers_frame[4] = HTTP_SERVER_FLAG_END_HEADERS;
	response_headers_frame[5] = 0x00;
	response_headers_frame[6] = 0x00;
	response_headers_frame[7] = 0x00;
	response_headers_frame[8] = new_stream_id & 0xFF;
	response_headers_frame[9] = hpack_status & 0xFF; /* HPACK :status */
	response_headers_frame[10] = 0x5a;
	response_headers_frame[11] = 0x04;
	response_headers_frame[12] = 0x67;
	response_headers_frame[13] = 0x7a;
	response_headers_frame[14] = 0x69;
	response_headers_frame[15] = 0x70;
	/* HPACK content-encoding: gzip */
}

void send_data(int socket_fd, const char *payload, size_t length, uint8_t type, uint8_t flags,
	       uint32_t stream_id)
{
	if (9 + length > HTTP_SERVER_MAX_FRAME_SIZE) {
		LOG_ERR("Payload is too large for the buffer");
		return;
	}

	uint8_t data_frame[HTTP_SERVER_MAX_FRAME_SIZE];

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

const char *get_frame_type_name(enum http_frame_type type)
{
	switch (type) {
	case HTTP_SERVER_DATA_FRAME:
		return "DATA";
	case HTTP_SERVER_HEADERS_FRAME:
		return "HEADERS";
	case HTTP_SERVER_PRIORITY_FRAME:
		return "PRIORITY";
	case HTTP_SERVER_RST_STREAM_FRAME:
		return "RST_STREAM";
	case HTTP_SERVER_SETTINGS_FRAME:
		return "SETTINGS";
	case HTTP_SERVER_PUSH_PROMISE_FRAME:
		return "PUSH_PROMISE";
	case HTTP_SERVER_PING_FRAME:
		return "PING";
	case HTTP_SERVER_GOAWAY_FRAME:
		return "GOAWAY";
	case HTTP_SERVER_WINDOW_UPDATE_FRAME:
		return "WINDOW_UPDATE";
	case HTTP_SERVER_CONTINUATION_FRAME:
		return "CONTINUATION";
	default:
		return "UNKNOWN";
	}
}

void print_http_frames(struct http_client_ctx *ctx_client)
{
	const char *bold = "\033[1m";
	const char *reset = "\033[0m";
	const char *green = "\033[32m";
	const char *blue = "\033[34m";

	struct http_frame *frame = &ctx_client->current_frame;
	int payload_received_length;

	LOG_DBG("%s=====================================%s", green, reset);
	LOG_DBG("%sReceived %s Frame :%s", bold, get_frame_type_name(frame->type), reset);
	LOG_DBG("  %sLength:%s %u", blue, reset, frame->length);
	LOG_DBG("  %sType:%s %u (%s)", blue, reset, frame->type, get_frame_type_name(frame->type));
	LOG_DBG("  %sFlags:%s %u", blue, reset, frame->flags);
	LOG_DBG("  %sStream Identifier:%s %u", blue, reset, frame->stream_identifier);
	printf("  %sPayload:%s ", blue, reset);

	if (ctx_client->offset > frame->length) {
		payload_received_length = frame->length;
	} else {
		payload_received_length = ctx_client->offset;
	}

	for (unsigned int j = 0; j < payload_received_length; j++) {
		printf("%02x ", frame->payload[j]);
	}
	printf("\n");
	LOG_DBG("%s=====================================%s", green, reset);
}

int parse_http_frame_header(struct http_client_ctx *ctx_client)
{

	unsigned char *buffer = ctx_client->buffer;
	unsigned long buffer_len = ctx_client->offset;
	struct http_frame *frame = &ctx_client->current_frame;

	frame->length = 0;
	frame->stream_identifier = 0;

	if (buffer_len < HTTP_SERVER_FRAME_HEADER_SIZE) {
		return 0;
	}

	frame->length = (buffer[HTTP_SERVER_FRAME_LENGTH_OFFSET] << 16) |
			(buffer[HTTP_SERVER_FRAME_LENGTH_OFFSET + 1] << 8) |
			buffer[HTTP_SERVER_FRAME_LENGTH_OFFSET + 2];
	frame->type = buffer[HTTP_SERVER_FRAME_TYPE_OFFSET];
	frame->flags = buffer[HTTP_SERVER_FRAME_FLAGS_OFFSET];
	frame->stream_identifier = (buffer[HTTP_SERVER_FRAME_STREAM_ID_OFFSET] << 24) |
				   (buffer[HTTP_SERVER_FRAME_STREAM_ID_OFFSET + 1] << 16) |
				   (buffer[HTTP_SERVER_FRAME_STREAM_ID_OFFSET + 2] << 8) |
				   buffer[HTTP_SERVER_FRAME_STREAM_ID_OFFSET + 3];
	frame->stream_identifier &= 0x7FFFFFFF;

	frame->payload = buffer;

	return 1;
}

bool settings_ack_flag(unsigned char flags)
{
	return (flags & HTTP_SERVER_FLAG_SETTINGS_ACK) != 0;
}

bool settings_end_headers_flag(unsigned char flags)
{
	return (flags & HTTP_SERVER_FLAG_END_HEADERS) != 0;
}

bool settings_end_stream_flag(unsigned char flags)
{
	return (flags & HTTP_SERVER_FLAG_END_STREAM) != 0;
}

void handle_post_request(struct http_server_ctx *ctx_server, char *request_payload)
{
#if defined(__ZEPHYR__)
	struct arithmetic_result ar;

	int ret = json_obj_parse(request_payload, strlen(request_payload), arithmetic_result_descr,
				 ARRAY_SIZE(arithmetic_result_descr), &ar);

	if (ret < 0) {
		return;
	}

	ar.result = ar.x + ar.y;

	if (ctx_server->results_count < POST_REQUEST_STORAGE_LIMIT) {
		ctx_server->results[ctx_server->results_count] = ar;
		ctx_server->results_count++;
	}
#else
	json_error_t error;
	json_t *root = json_loads(request_payload, 0, &error);

	if (!root) {
		fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
		return;
	}

	json_t *x_json = json_object_get(root, "x");
	json_t *y_json = json_object_get(root, "y");

	if (!json_is_integer(x_json) || !json_is_integer(y_json)) {
		fprintf(stderr, "error: x or y is not an integer or doesn't exist.\n");
		json_decref(root);
		return;
	}

	int x = json_integer_value(x_json);
	int y = json_integer_value(y_json);

	if (ctx_server->results_count < POST_REQUEST_STORAGE_LIMIT) {
		ctx_server->results[ctx_server->results_count].x = x;
		ctx_server->results[ctx_server->results_count].y = y;
		ctx_server->results[ctx_server->results_count].result = x + y;
		ctx_server->results_count++;
	}

	json_decref(root);
#endif
}

void handle_get_request(struct http_server_ctx *ctx_server, int client)
{
#if defined(__ZEPHYR__)
	char json_response[1024] = "[";

	for (int i = 0; i < ctx_server->results_count; i++) {
		char entry[128];

		int len = json_obj_encode_buf(arithmetic_result_descr,
					      ARRAY_SIZE(arithmetic_result_descr),
					      &ctx_server->results[i], entry, sizeof(entry));

		if (len < 0) {
			return;
		}

		strcat(json_response, entry);

		if (i != ctx_server->results_count - 1) {
			strcat(json_response, ", ");
		}
	}

	strcat(json_response, "]");
#else
	json_t *root = json_array();

	for (int i = 0; i < ctx_server->results_count; i++) {
		json_t *item = json_object();

		json_object_set_new(item, "x", json_integer(ctx_server->results[i].x));
		json_object_set_new(item, "y", json_integer(ctx_server->results[i].y));
		json_object_set_new(item, "result", json_integer(ctx_server->results[i].result));
		json_array_append_new(root, item);
	}

	char *json_response = json_dumps(root, 0);

	json_decref(root);
#endif

	char header[HTTP_SERVER_MAX_RESPONSE_SIZE];

	snprintf(header, sizeof(header),
		 "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n",
		 strlen(json_response));
	sendall(client, header, strlen(header));
	sendall(client, json_response, strlen(json_response));
}
