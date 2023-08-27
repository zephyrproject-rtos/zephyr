/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "headers/http_service.h"
#include "headers/server_functions.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/tls_credentials.h>
LOG_MODULE_REGISTER(net_http_server, LOG_LEVEL_DBG);

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
#include <zephyr/sys/byteorder.h>

#else

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/sys/eventfd.h>

#endif

static char url_buffer[CONFIG_NET_HTTP_SERVER_MAX_URL_LENGTH];
static char http_response[CONFIG_NET_HTTP_SERVER_MAX_RESPONSE_SIZE];
static const char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

static struct http_parser_settings parserSettings;
static struct http_parser parser;

static struct arithmetic_result results[POST_REQUEST_STORAGE_LIMIT];
static int results_count = 1;

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

static const char content_404[] = {
#ifdef INCLUDE_HTML_CONTENT
#include "not_found_page.html.gz.inc"
#endif
};

#define INVALID_SOCK -1

bool has_upgrade_header = true;

int http_server_init(struct http_server_ctx *ctx)
{
	int i = 1;
	uint8_t af;
	socklen_t len;
	struct sockaddr_in6 addr_storage;
	const union {
		struct sockaddr *addr;
		struct sockaddr_in *addr4;
		struct sockaddr_in6 *addr6;
	} _addr = {.addr = (struct sockaddr *)&addr_storage};
	int proto = IPPROTO_TCP;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	proto = IPPROTO_TLS_1_2;
#endif

	HTTP_SERVICE_FOREACH(svc)
	{
		/* set the default address (in6addr_any / INADDR_ANY are all 0) */
		addr_storage = (struct sockaddr_in6){0};

		/* Set up the server address struct according to address family */
		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    inet_pton(AF_INET6, svc->host, &_addr.addr6->sin6_addr) == 1) {
			/* if a literal IPv6 address is provided as the host, use IPv6 */
			af = AF_INET6;
			len = sizeof(*_addr.addr6);

			_addr.addr6->sin6_family = AF_INET6;
			_addr.addr6->sin6_port = *svc->port;
		} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
			   inet_pton(AF_INET, svc->host, &_addr.addr4->sin_addr) == 1) {
			/* if a literal IPv4 address is provided as the host, use IPv4 */
			af = AF_INET;
			len = sizeof(*_addr.addr4);

			_addr.addr4->sin_family = AF_INET;
			_addr.addr4->sin_port = *svc->port;
		} else if (IS_ENABLED(CONFIG_NET_IPV6)) {
			/* prefer IPv6 if both IPv6 and IPv4 are supported */
			af = AF_INET6;
			len = sizeof(*_addr.addr6);

			_addr.addr6->sin6_family = AF_INET6;
			_addr.addr6->sin6_port = *svc->port;
		} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
			af = AF_INET;
			len = sizeof(*_addr.addr4);

			_addr.addr4->sin_family = AF_INET;
			_addr.addr4->sin_port = *svc->port;
		} else {
			LOG_ERR("Neither IPv4 nor IPv6 is enabled");
			break;
		}
		/* Create a socket */
		ctx->server_fd = socket(af, SOCK_STREAM, proto);
		if (ctx->server_fd < 0) {
			LOG_ERR("socket: %d", errno);
			break;
		}
		if (setsockopt(ctx->server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) <
		    0) {
			LOG_ERR("setsockopt: %d", errno);
			break;
		}

#if defined(CONFIG_TLS_CREDENTIALS) || defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		static const sec_tag_t server_tag_list_verify_none[] = {
			HTTP_SERVER_SERVER_CERTIFICATE_TAG,
		};

		const sec_tag_t *sec_tag_list;
		size_t sec_tag_list_size;

		sec_tag_list = server_tag_list_verify_none;
		sec_tag_list_size = sizeof(server_tag_list_verify_none);

		if (setsockopt(ctx->server_fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
			       sec_tag_list_size) < 0) {
			LOG_ERR("setsockopt");
			return -errno;
		}

		if (setsockopt(ctx->server_fd, SOL_TLS, TLS_HOSTNAME, "localhost",
			       sizeof("localhost")) < 0) {
			LOG_ERR("setsockopt");
			return -errno;
		}
#endif

		/* Bind to the specified address */
		if (bind(ctx->server_fd, _addr.addr, len) < 0) {
			LOG_ERR("bind: %d", errno);
			break;
		}
		if (*svc->port == 0) {
			/* ephemeral port - read back the port number */
			len = sizeof(addr_storage);
			if (getsockname(ctx->server_fd, _addr.addr, &len) < 0) {
				LOG_ERR("getsockname: %d", errno);
				break;
			}
			*svc->port = htons(_addr.addr4->sin_port);
		}
		/* Listen for connections */
		if (listen(ctx->server_fd, MAX_CLIENTS) < 0) {
			LOG_ERR("listen: %d", errno);
			break;
		}
		LOG_INF("Initialized HTTP Service %d http://%s:%u", i, svc->host,
			htons(*svc->port));
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

	for (int j = 0; j < ARRAY_SIZE(ctx->fds); j++) {
		ctx->fds[j].fd = INVALID_SOCK;
	}

	ctx->fds[0].fd = ctx->server_fd;
	ctx->fds[0].events = POLLIN;

	ctx->fds[1].fd = ctx->event_fd;
	ctx->fds[1].events = POLLIN;

	ctx->num_clients = 0;

	return ctx->server_fd;
}

int accept_new_client(int server_fd)
{
	int new_socket;
	socklen_t addrlen;
	struct sockaddr_storage sa;

	memset(&sa, 0, sizeof(sa));
	addrlen = sizeof(sa);

	new_socket = accept(server_fd, (struct sockaddr *)&sa, &addrlen);

	if (new_socket < 0) {
		LOG_ERR("accept failed");
		return new_socket;
	}

	return new_socket;
}

int http_server_start(struct http_server_ctx *ctx)
{
	eventfd_t value = 0;

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

			if (ret == 1 && ctx->fds[ret].revents) {
				eventfd_read(ctx->event_fd, &value);
				LOG_DBG("Received stop event. exiting ..");
				return 0;
			}

			if (!(ctx->fds[i].revents & POLLIN)) {
				continue;
			}

			if (i == 0) {
				int new_socket = accept_new_client(ctx->server_fd);

				bool found_slot = false;

				for (int j = 2; j < MAX_CLIENTS + 1; j++) {
					if (ctx->fds[j].fd != INVALID_SOCK) {
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

					close(new_socket);
				}
				continue;
			}

			int valread = recv(client->client_fd, client->buffer + client->offset,
					   sizeof(client->buffer) - client->offset, 0);

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
	} while (1);
	return 0;
}

int http_server_stop(struct http_server_ctx *ctx)
{
	eventfd_write(ctx->event_fd, 1);

	return 0;
}

void close_client_connection(struct http_server_ctx *ctx_server, int client_index)
{
	close(ctx_server->fds[client_index].fd);
	ctx_server->fds[client_index].fd = INVALID_SOCK;
	ctx_server->fds[client_index].events = 0;
	ctx_server->fds[client_index].revents = 0;

	if (client_index == ctx_server->num_clients) {
		while (ctx_server->num_clients > 0 &&
		       ctx_server->fds[ctx_server->num_clients].fd == INVALID_SOCK) {
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

struct http_resource_detail *get_resource_detail(const char *path)
{
	HTTP_SERVICE_FOREACH(service)
	{
		HTTP_SERVICE_FOREACH_RESOURCE(service, resource)
		{
			if (strcmp(resource->resource, path) == 0) {
				return resource->detail;
			}
		}
	}
	return NULL;
}

int handle_http1_static_resource(struct http_resource_detail_static *static_detail, int client_fd)
{
	const char *data;
	int len;
	int ret;

	if (static_detail->common.bitmask_of_supported_http_methods & GET) {
		data = static_detail->static_data;
		len = static_detail->static_data_len;

		sprintf(http_response,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Encoding: gzip\r\n"
			"Content-Length: %d\r\n\r\n",
			len);

		ret = sendall(client_fd, http_response, strlen(http_response));
		if (ret < 0) {
			return ret;
		}

		ret = sendall(client_fd, data, len);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

int handle_http1_rest_resource(struct http_resource_detail_static *static_detail,
			       struct http_client_ctx *ctx_client)
{

	if (static_detail->common.bitmask_of_supported_http_methods & POST) {
		char *json_start = strstr(ctx_client->buffer, "\r\n\r\n");

		if (json_start) {
			handle_post_request(json_start, ctx_client->client_fd);
		}
	}

	return 0;
}

int handle_http1_request(struct http_server_ctx *ctx_server, struct http_client_ctx *ctx_client,
			 int client_index)
{
	LOG_DBG("HTTP_SERVER_REQUEST.");
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

		return 0;
	}

	struct http_resource_detail *detail = get_resource_detail(url_buffer);

	if (detail != NULL) {
		if (detail->type == HTTP_RESOURCE_TYPE_STATIC) {
			int ret = handle_http1_static_resource(
				(struct http_resource_detail_static *)detail,
				ctx_client->client_fd);
			if (ret < 0) {
				close_client_connection(ctx_server, client_index);
				return ret;
			}
		} else if (detail->type == HTTP_RESOURCE_TYPE_REST) {

			handle_http1_rest_resource((struct http_resource_detail_static *)detail,
						   ctx_client);
		}
	} else {
		const char *not_found_response = "HTTP/1.1 404 Not Found\r\n"
						 "Content-Length: 9\r\n\r\n"
						 "Not Found";
		if (sendall(ctx_client->client_fd, not_found_response, strlen(not_found_response)) <
		    0) {
			close_client_connection(ctx_server, client_index);
		}
	}

	close_client_connection(ctx_server, client_index);
	memset(ctx_client->buffer, 0, sizeof(ctx_client->buffer));
	ctx_client->offset = 0;

	return 0;
}

int handle_http_done(struct http_server_ctx *ctx_server, struct http_client_ctx *ctx_client,
		     int client_index)
{
	LOG_DBG("HTTP_SERVER_DONE_STATE");

	close_client_connection(ctx_server, client_index);

	return -errno;
}

int handle_http2_static_resource(struct http_resource_detail_static *static_detail,
				 struct http_frame *frame, int client_fd)
{
	if (static_detail->common.bitmask_of_supported_http_methods & GET) {
		const char *content_200 = static_detail->static_data;
		size_t content_size = static_detail->static_data_len;

		int ret = send_headers_frame(client_fd, HTTP_SERVER_HPACK_STATUS_2OO,
					     frame->stream_identifier);
		if (ret < 0) {
			LOG_ERR("ERROR writing to socket");
			return ret;
		}

		ret = send_data_frame(client_fd, content_200, content_size,
				      frame->stream_identifier);
		if (ret < 0) {
			LOG_ERR("ERROR writing to socket");
			return ret;
		}
	}
	return 0;
}

int handle_http_frame_headers(struct http_client_ctx *ctx_client)
{
	LOG_DBG("HTTP_SERVER_FRAME_HEADERS");

	struct http_frame *frame = &ctx_client->current_frame;
	int ret;

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

	struct http_resource_detail *detail = get_resource_detail(path);

	if (detail != NULL) {
		if (detail->type == HTTP_RESOURCE_TYPE_STATIC) {

			ret = handle_http2_static_resource(
				(struct http_resource_detail_static *)detail, frame,
				ctx_client->client_fd);
			if (ret < 0) {
				return ret;
			}
		}
	} else {
		ret = send_headers_frame(ctx_client->client_fd, HTTP_SERVER_HPACK_STATUS_4O4,
					 frame->stream_identifier);
		if (ret < 0) {
			LOG_ERR("ERROR writing to socket");
			return ret;
		}

		ret = send_data_frame(ctx_client->client_fd, content_404, sizeof(content_404),
				      frame->stream_identifier);
		if (ret < 0) {
			LOG_ERR("ERROR writing to socket");
			return ret;
		}
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
		size_t out_len = send(sock, buf, len, 0);

		if (out_len < 0) {
			return out_len;
		}

		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
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

void encode_frame_header(uint8_t *buf, uint32_t payload_len, enum http_frame_type frame_type,
			 uint8_t flags, uint32_t stream_id)
{
	sys_put_be24(payload_len, &buf[HTTP_SERVER_FRAME_LENGTH_OFFSET]);
	buf[HTTP_SERVER_FRAME_TYPE_OFFSET] = frame_type;
	buf[HTTP_SERVER_FRAME_FLAGS_OFFSET] = flags;
	sys_put_be32(stream_id, &buf[HTTP_SERVER_FRAME_STREAM_ID_OFFSET]);
}

int send_headers_frame(int socket_fd, uint8_t hpack_status, uint32_t stream_id)
{
	uint8_t frame_header[HTTP_SERVER_FRAME_HEADER_SIZE];
	/* TODO For now payload is hardcoded, but it should be possible to
	 * generate headers dynamically (need hpack encoder).
	 */
	uint8_t headers_payload[] = {
		/* HPACK :status */
		hpack_status,
		/* HPACK content-encoding: gzip */
		0x5a,
		0x04,
		0x67,
		0x7a,
		0x69,
		0x70,
	};

	encode_frame_header(frame_header, sizeof(headers_payload), HTTP_SERVER_HEADERS_FRAME,
			    HTTP_SERVER_FLAG_END_HEADERS, stream_id);

	if (sendall(socket_fd, frame_header, sizeof(frame_header)) < 0) {
		LOG_ERR("ERROR writing to socket");
		return -errno;
	}

	if (sendall(socket_fd, headers_payload, sizeof(headers_payload)) < 0) {
		LOG_ERR("ERROR writing to socket");
		return -errno;
	}

	return 0;
}

int send_data_frame(int socket_fd, const char *payload, size_t length, uint32_t stream_id)
{
	uint8_t frame_header[HTTP_SERVER_FRAME_HEADER_SIZE];

	encode_frame_header(frame_header, length, HTTP_SERVER_DATA_FRAME,
			    HTTP_SERVER_FLAG_END_STREAM, stream_id);

	if (sendall(socket_fd, frame_header, sizeof(frame_header)) < 0) {
		LOG_ERR("ERROR writing to socket");
		return -errno;
	}

	if (sendall(socket_fd, payload, length) < 0) {
		LOG_ERR("ERROR writing to socket");
		return -errno;
	}

	return 0;
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

	if (ctx_client->offset > frame->length) {
		payload_received_length = frame->length;
	} else {
		payload_received_length = ctx_client->offset;
	}

	LOG_HEXDUMP_DBG(frame->payload, payload_received_length, "Payload");
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

void handle_post_request(char *request_payload, int client)
{
	const struct json_obj_descr arithmetic_result_descr[] = {
		JSON_OBJ_DESCR_PRIM(struct arithmetic_result, x, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct arithmetic_result, y, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct arithmetic_result, result, JSON_TOK_NUMBER),
	};
	struct arithmetic_result ar;

	int ret = json_obj_parse(request_payload, strlen(request_payload), arithmetic_result_descr,
				 ARRAY_SIZE(arithmetic_result_descr), &ar);

	if (ret < 0) {
		return;
	}

	ar.result = ar.x + ar.y;

	if (results_count < POST_REQUEST_STORAGE_LIMIT) {
		results[results_count - 1] = ar;
	}

	char json_response[1024] = "[";

	for (int i = 0; i < results_count; i++) {
		char entry[128];

		int len = json_obj_encode_buf(arithmetic_result_descr,
					      ARRAY_SIZE(arithmetic_result_descr), &results[i],
					      entry, sizeof(entry));

		if (len < 0) {
			return;
		}

		strcat(json_response, entry);

		if (i != results_count - 1) {
			strcat(json_response, ", ");
		}
	}

	strcat(json_response, "]");

	if (results_count < POST_REQUEST_STORAGE_LIMIT) {
		results_count++;
	}

	char header[HTTP_SERVER_MAX_RESPONSE_SIZE];

	snprintf(header, sizeof(header),
		 "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n",
		 strlen(json_response));

	sendall(client, header, strlen(header));
	sendall(client, json_response, strlen(json_response));
}
