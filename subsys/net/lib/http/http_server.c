/*
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2023 Nordic Semiconductor ASA
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
LOG_MODULE_REGISTER(net_http_server, CONFIG_NET_HTTP_SERVER_LOG_LEVEL);

#define HTTP_SERVER_MAX_RESPONSE_SIZE CONFIG_NET_HTTP_SERVER_MAX_RESPONSE_SIZE
#define HTTP_SERVER_MAX_URL_LENGTH    CONFIG_NET_HTTP_SERVER_MAX_URL_LENGTH
#define HTTP_SERVER_MAX_FRAME_SIZE    CONFIG_NET_HTTP_SERVER_MAX_FRAME_SIZE

#define TEMP_BUF_LEN 64

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/sys/eventfd.h>

#include "../../ip/net_private.h"

static const char preface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

static const char final_chunk[] = "0\r\n\r\n";
static const char *crlf = &final_chunk[3];

static const unsigned char settings_frame[9] = {
	0x00, 0x00, 0x00,        /* Length */
	0x04,                    /* Type: 0x04 - setting frames for config or acknowledgment */
	0x00,                    /* Flags: 0x00 - unused flags */
	0x00, 0x00, 0x00, 0x00}; /* Reserved, Stream Identifier: 0x00 - overall connection */

static const unsigned char settings_ack[9] = {
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

int http_server_init(struct http_server_ctx *ctx)
{
	int proto;
	int failed = 0, count = 0;
	int svc_count;
	socklen_t len;
	int fd, af, i;
	struct sockaddr_storage addr_storage;
	const union {
		struct sockaddr *addr;
		struct sockaddr_in *addr4;
		struct sockaddr_in6 *addr6;
	} addr = {
		.addr = (struct sockaddr *)&addr_storage
	};

	HTTP_SERVICE_COUNT(&svc_count);

	/* Initialize fds */
	memset(ctx->fds, 0, sizeof(ctx->fds));
	memset(ctx->clients, 0, sizeof(ctx->clients));

	for (i = 0; i < ARRAY_SIZE(ctx->fds); i++) {
		ctx->fds[i].fd = INVALID_SOCK;
	}

	/* Create an eventfd that can be used to trigger events during polling */
	fd = eventfd(0, 0);
	if (fd < 0) {
		fd = -errno;
		LOG_ERR("eventfd failed (%d)", fd);
		return fd;
	}

	ctx->fds[count].fd = fd;
	ctx->fds[count].events = POLLIN;
	count++;

	HTTP_SERVICE_FOREACH(svc) {
		/* set the default address (in6addr_any / INADDR_ANY are all 0) */
		memset(&addr_storage, sizeof(struct sockaddr_storage), 0);

		/* Set up the server address struct according to address family */
		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    zsock_inet_pton(AF_INET6, svc->host, &addr.addr6->sin6_addr) == 1) {
			af = AF_INET6;
			len = sizeof(*addr.addr6);

			addr.addr6->sin6_family = AF_INET6;
			addr.addr6->sin6_port = htons(*svc->port);
		} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
			   zsock_inet_pton(AF_INET, svc->host, &addr.addr4->sin_addr) == 1) {
			af = AF_INET;
			len = sizeof(*addr.addr4);

			addr.addr4->sin_family = AF_INET;
			addr.addr4->sin_port = htons(*svc->port);
		} else if (IS_ENABLED(CONFIG_NET_IPV6)) {
			/* prefer IPv6 if both IPv6 and IPv4 are supported */
			af = AF_INET6;
			len = sizeof(*addr.addr6);

			addr.addr6->sin6_family = AF_INET6;
			addr.addr6->sin6_port = htons(*svc->port);
		} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
			af = AF_INET;
			len = sizeof(*addr.addr4);

			addr.addr4->sin_family = AF_INET;
			addr.addr4->sin_port = htons(*svc->port);
		} else {
			LOG_ERR("Neither IPv4 nor IPv6 is enabled");
			failed++;
			break;
		}

		/* Create a socket */
		if (COND_CODE_1(CONFIG_NET_SOCKETS_SOCKOPT_TLS,
				(svc->sec_tag_list != NULL),
				(0))) {
			proto = IPPROTO_TLS_1_2;
		} else {
			proto = IPPROTO_TCP;
		}

		fd = zsock_socket(af, SOCK_STREAM, proto);
		if (fd < 0) {
			LOG_ERR("socket: %d", errno);
			failed++;
			continue;
		}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		if (svc->sec_tag_list != NULL) {
			if (zsock_setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST,
					     svc->sec_tag_list,
					     svc->sec_tag_list_size) < 0) {
				LOG_ERR("setsockopt: %d", errno);
				zsock_close(fd);
				continue;
			}

			if (zsock_setsockopt(fd, SOL_TLS, TLS_HOSTNAME, "localhost",
					     sizeof("localhost")) < 0) {
				LOG_ERR("setsockopt: %d", errno);
				zsock_close(fd);
				continue;
			}
		}
#endif

		if (zsock_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1},
				     sizeof(int)) < 0) {
			LOG_ERR("setsockopt: %d", errno);
			zsock_close(fd);
			continue;
		}

		if (zsock_bind(fd, addr.addr, len) < 0) {
			LOG_ERR("bind: %d", errno);
			failed++;
			zsock_close(fd);
			continue;
		}

		if (*svc->port == 0) {
			/* ephemeral port - read back the port number */
			len = sizeof(addr_storage);
			if (zsock_getsockname(fd, addr.addr, &len) < 0) {
				LOG_ERR("getsockname: %d", errno);
				zsock_close(fd);
				continue;
			}

			*svc->port = ntohs(addr.addr4->sin_port);
		}

		if (zsock_listen(fd, MAX_CLIENTS) < 0) {
			LOG_ERR("listen: %d", errno);
			failed++;
			zsock_close(fd);
			continue;
		}

		LOG_DBG("Initialized HTTP Service %s:%u", svc->host, *svc->port);

		ctx->fds[count].fd = fd;
		ctx->fds[count].events = POLLIN;
		count++;
	}

	if (failed >= svc_count) {
		LOG_ERR("All services failed (%d)", failed);
		return -ESRCH;
	}

	ctx->listen_fds = count;
	ctx->num_clients = 0;

	return 0;
}

int accept_new_client(int server_fd)
{
	int new_socket;
	socklen_t addrlen;
	struct sockaddr_storage sa;

	memset(&sa, 0, sizeof(sa));
	addrlen = sizeof(sa);

	new_socket = zsock_accept(server_fd, (struct sockaddr *)&sa, &addrlen);
	if (new_socket < 0) {
		new_socket = -errno;
		LOG_DBG("[%d] accept failed (%d)", server_fd, new_socket);
		return new_socket;
	}

	LOG_DBG("New client from %s:%d",
		net_sprint_addr(sa.ss_family, &net_sin((struct sockaddr *)&sa)->sin_addr),
		ntohs(net_sin((struct sockaddr *)&sa)->sin_port));

	return new_socket;
}

int http_server_start(struct http_server_ctx *ctx)
{
	struct http_client_ctx *client;
	eventfd_t value;
	bool found_slot;
	int new_socket;
	int ret, i, j;

	value = 0;

	while (1) {
		ret = zsock_poll(ctx->fds, ctx->listen_fds + ctx->num_clients, -1);
		if (ret < 0) {
			ret = -errno;
			LOG_DBG("poll failed (%d)", ret);
			return ret;
		}

		if (ret == 0) {
			/* should not happen because timeout is -1 */
			break;
		}

		if (ret == 1 && ctx->fds[0].revents) {
			eventfd_read(ctx->fds[0].fd, &value);
			LOG_DBG("Received stop event. exiting ..");
			return 0;
		}

		for (i = 0; i < ARRAY_SIZE(ctx->fds); i++) {
			if (ctx->fds[i].fd < 0) {
				continue;
			}

			if (ctx->fds[i].revents & POLLERR) {
				LOG_DBG("Error on fd %d", ctx->fds[i].fd);
				continue;
			}

			if (ctx->fds[i].revents & POLLHUP) {
				if (i >= ctx->listen_fds) {
					LOG_DBG("Client #%d has disconnected",
						i - ctx->listen_fds);

					client = &ctx->clients[i - ctx->listen_fds];
					close_client_connection(ctx, client);
				}

				continue;
			}

			if (!(ctx->fds[i].revents & POLLIN)) {
				continue;
			}

			/* First check if we have something to accept */
			if (i < ctx->listen_fds) {
				new_socket = accept_new_client(ctx->fds[i].fd);
				if (new_socket < 0) {
					ret = -errno;
					LOG_DBG("accept: %d", ret);
					continue;
				}

				found_slot = false;

				for (j = ctx->listen_fds; j < ARRAY_SIZE(ctx->fds); j++) {
					if (ctx->fds[j].fd != INVALID_SOCK) {
						continue;
					}

					ctx->fds[j].fd = new_socket;
					ctx->fds[j].events = POLLIN;

					ctx->num_clients++;

					LOG_DBG("Init client #%d", j - ctx->listen_fds);

					init_client_ctx(&ctx->clients[j - ctx->listen_fds],
							new_socket);
					found_slot = true;
					break;
				}

				if (!found_slot) {
					LOG_DBG("No free slot found.");
					zsock_close(new_socket);
				}
			} else {
				client = &ctx->clients[i - ctx->listen_fds];

				ret = zsock_recv(client->fd,
						 client->buffer + client->offset,
						 sizeof(client->buffer) - client->offset, 0);
				if (ret <= 0) {
					if (ret == 0) {
						LOG_DBG("Connection closed by peer for client #%d",
							i - ctx->listen_fds);
					} else {
						ret = -errno;
						LOG_DBG("ERROR reading from socket (%d)", ret);
					}

					close_client_connection(ctx, client);
					continue;
				}

				client->offset += ret;
				handle_http_request(ctx, client);
			}
		}
	}

	return 0;
}

int http_server_stop(struct http_server_ctx *ctx)
{
	eventfd_write(ctx->fds[0].fd, 1);

	return 0;
}

void close_client_connection(struct http_server_ctx *server,
			     struct http_client_ctx *client)
{
	int i;

	__ASSERT_NO_MSG(IS_ARRAY_ELEMENT(server->clients, client));

	zsock_close(client->fd);

	server->num_clients--;

	for (i = server->listen_fds; i < ARRAY_SIZE(server->fds); i++) {
		if (server->fds[i].fd == client->fd) {
			server->fds[i].fd = INVALID_SOCK;
			break;
		}
	}

	memset(client, 0, sizeof(struct http_client_ctx));
}

void init_client_ctx(struct http_client_ctx *client, int new_socket)
{
	int i;

	client->fd = new_socket;
	client->offset = 0;
	client->server_state = HTTP_SERVER_PREFACE_STATE;
	client->has_upgrade_header = false;

	memset(client->buffer, 0, sizeof(client->buffer));
	memset(client->url_buffer, 0, sizeof(client->url_buffer));

	for (i = 0; i < MAX_STREAMS; i++) {
		client->streams[i].stream_state = HTTP_SERVER_STREAM_IDLE;
		client->streams[i].stream_id = 0;
	}
}

struct http_stream_ctx *find_http_stream_context(struct http_client_ctx *client,
						 uint32_t stream_id)
{
	int i;

	for (i = 0; i < MAX_STREAMS; i++) {
		if (client->streams[i].stream_id == stream_id) {
			return &client->streams[i];
		}
	}

	return NULL;
}

struct http_stream_ctx *allocate_http_stream_context(struct http_client_ctx *client,
						     uint32_t stream_id)
{
	int i;

	for (i = 0; i < MAX_STREAMS; i++) {
		if (client->streams[i].stream_state == HTTP_SERVER_STREAM_IDLE) {
			client->streams[i].stream_id = stream_id;
			client->streams[i].stream_state = HTTP_SERVER_STREAM_OPEN;
			return &client->streams[i];
		}
	}

	return NULL;
}

int handle_http_request(struct http_server_ctx *server, struct http_client_ctx *client)
{
	int ret = -EINVAL;

	do {
		switch (client->server_state) {
		case HTTP_SERVER_PREFACE_STATE:
			ret = handle_http_preface(client);
			break;
		case HTTP_SERVER_REQUEST_STATE:
			ret = handle_http1_request(server, client);
			break;
		case HTTP_SERVER_FRAME_HEADER_STATE:
			ret = handle_http_frame_header(server, client);
			break;
		case HTTP_SERVER_FRAME_HEADERS_STATE:
			ret = handle_http_frame_headers(client);
			break;
		case HTTP_SERVER_FRAME_CONTINUATION_STATE:
			ret = handle_http_frame_continuation(client);
			break;
		case HTTP_SERVER_FRAME_SETTINGS_STATE:
			ret = handle_http_frame_settings(client);
			break;
		case HTTP_SERVER_FRAME_WINDOW_UPDATE_STATE:
			ret = handle_http_frame_window_update(client);
			break;
		case HTTP_SERVER_FRAME_RST_STREAM_STATE:
			ret = handle_http_frame_rst_frame(server, client);
			break;
		case HTTP_SERVER_FRAME_GOAWAY_STATE:
			ret = handle_http_frame_goaway(server, client);
			break;
		case HTTP_SERVER_FRAME_PRIORITY_STATE:
			ret = handle_http_frame_priority(client);
			break;
		case HTTP_SERVER_DONE_STATE:
			ret = handle_http_done(server, client);
			break;
		default:
			ret = handle_http_done(server, client);
			break;
		}
	} while (ret == 0 && client->offset > 0);

	return ret;
}

int handle_http_frame_header(struct http_server_ctx *server,
			     struct http_client_ctx *client)
{
	int bytes_consumed;
	int parse_result;

	LOG_DBG("HTTP_SERVER_FRAME_HEADER");

	parse_result = parse_http_frame_header(client);
	if (parse_result == 0) {
		return -EAGAIN;
	} else if (parse_result < 0) {
		return parse_result;
	}

	bytes_consumed = HTTP_SERVER_FRAME_HEADER_SIZE;

	client->offset -= bytes_consumed;
	memmove(client->buffer, client->buffer + bytes_consumed, client->offset);

	switch (client->current_frame.type) {
	case HTTP_SERVER_HEADERS_FRAME:
		return enter_http_frame_headers_state(server, client);
	case HTTP_SERVER_CONTINUATION_FRAME:
		return enter_http_frame_continuation_state(client);
	case HTTP_SERVER_SETTINGS_FRAME:
		return enter_http_frame_settings_state(client);
	case HTTP_SERVER_WINDOW_UPDATE_FRAME:
		return enter_http_frame_window_update_state(client);
	case HTTP_SERVER_RST_STREAM_FRAME:
		return enter_http_frame_rst_stream_state(server, client);
	case HTTP_SERVER_GOAWAY_FRAME:
		return enter_http_frame_goaway_state(server, client);
	case HTTP_SERVER_PRIORITY_FRAME:
		return enter_http_frame_priority_state(client);
	default:
		return enter_http_http_done_state(server, client);
	}

	return 0;
}

int enter_http_frame_settings_state(struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_FRAME_SETTINGS_STATE;

	return 0;
}

int enter_http_frame_headers_state(struct http_server_ctx *server,
				   struct http_client_ctx *client)
{
	struct http_frame *frame = &client->current_frame;
	struct http_stream_ctx *stream;

	stream = find_http_stream_context(client, frame->stream_identifier);
	if (!stream) {
		LOG_DBG("|| stream ID ||  %d", frame->stream_identifier);

		stream = allocate_http_stream_context(client, frame->stream_identifier);
		if (!stream) {
			LOG_DBG("No available stream slots. Connection closed.");
			close_client_connection(server, client);

			return -ENOMEM;
		}
	}

	if (settings_end_headers_flag(client->current_frame.flags) &&
	    settings_end_stream_flag(client->current_frame.flags)) {
		client->server_state = HTTP_SERVER_FRAME_HEADERS_STATE;
	} else {
		client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;
	}

	return 0;
}

int enter_http_frame_continuation_state(struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_FRAME_CONTINUATION_STATE;

	return 0;
}

int enter_http_frame_window_update_state(struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_FRAME_WINDOW_UPDATE_STATE;

	return 0;
}

int enter_http_frame_priority_state(struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_FRAME_PRIORITY_STATE;

	return 0;
}

int enter_http_frame_rst_stream_state(struct http_server_ctx *server,
				      struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_FRAME_RST_STREAM_STATE;

	return 0;
}

int enter_http_frame_goaway_state(struct http_server_ctx *server,
				  struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_FRAME_GOAWAY_STATE;

	return 0;
}

int enter_http_http_done_state(struct http_server_ctx *server,
			       struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_DONE_STATE;

	return 0;
}

int handle_http_preface(struct http_client_ctx *client)
{
	LOG_DBG("HTTP_SERVER_PREFACE_STATE.");

	if (client->offset < sizeof(preface) - 1) {
		/* We don't have full preface yet, get more data. */
		return -EAGAIN;
	}

	if (strncmp(client->buffer, preface, sizeof(preface) - 1) != 0) {
		client->server_state = HTTP_SERVER_REQUEST_STATE;
	} else {
		client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;
		client->offset -= sizeof(preface) - 1;

		memmove(client->buffer, client->buffer + sizeof(preface) - 1,
			client->offset);
	}

	return 0;
}

struct http_resource_detail *get_resource_detail(const char *path,
						 int *path_len)
{
	HTTP_SERVICE_FOREACH(service) {
		HTTP_SERVICE_FOREACH_RESOURCE(service, resource) {
			int len = strlen(resource->resource);

			if (strncmp(path, resource->resource, len) == 0) {
				*path_len = len;
				return resource->detail;
			}
		}
	}

	return NULL;
}

int handle_http1_static_resource(struct http_resource_detail_static *static_detail,
				 int client_fd)
{
#define RESPONSE_TEMPLATE			\
	"HTTP/1.1 200 OK\r\n"			\
	"Content-Type: text/html\r\n"		\
	"Content-Encoding: gzip\r\n"		\
	"Content-Length: %d\r\n\r\n"

	/* Add couple of bytes to total response */
	char http_response[sizeof(RESPONSE_TEMPLATE) + sizeof("xxxx")];
	const char *data;
	int len;
	int ret;

	if (static_detail->common.bitmask_of_supported_http_methods & BIT(HTTP_GET)) {
		data = static_detail->static_data;
		len = static_detail->static_data_len;

		snprintk(http_response, sizeof(http_response), RESPONSE_TEMPLATE, len);

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

#define RESPONSE_TEMPLATE_CHUNKED			\
	"HTTP/1.1 200 OK\r\n"				\
	"Content-Type: text/html\r\n"			\
	"Transfer-Encoding: chunked\r\n\r\n"

#define RESPONSE_TEMPLATE_DYNAMIC			\
	"HTTP/1.1 200 OK\r\n"				\
	"Content-Type: text/html\r\n\r\n"		\

static int dynamic_get_req(struct http_resource_detail_dynamic *dynamic_detail,
			   struct http_client_ctx *client)
{
	/* offset tells from where the GET params start */
	int ret, remaining, offset = dynamic_detail->common.path_len;
	char *ptr;
	char tmp[TEMP_BUF_LEN];

	ret = sendall(client->fd, RESPONSE_TEMPLATE_CHUNKED,
		      sizeof(RESPONSE_TEMPLATE_CHUNKED) - 1);
	if (ret < 0) {
		return ret;
	}

	remaining = strlen(&client->url_buffer[dynamic_detail->common.path_len]);

	/* Pass URL to the client */
	while (1) {
		int copy_len, send_len;

		ptr = &client->url_buffer[offset];
		copy_len = MIN(remaining, dynamic_detail->data_buffer_len);

		memcpy(dynamic_detail->data_buffer, ptr, copy_len);

	again:
		send_len = dynamic_detail->cb(client, dynamic_detail->data_buffer,
					      copy_len, dynamic_detail->user_data);
		if (send_len > 0) {
			ret = snprintk(tmp, sizeof(tmp), "%x\r\n", send_len);
			ret = sendall(client->fd, tmp, ret);
			if (ret < 0) {
				return ret;
			}

			ret = sendall(client->fd, dynamic_detail->data_buffer, send_len);
			if (ret < 0) {
				return ret;
			}

			(void)sendall(client->fd, crlf, 2);

			offset += copy_len;
			remaining -= copy_len;

			/* If we have passed all the data to the application,
			 * then just pass empty buffer to it.
			 */
			if (remaining == 0) {
				copy_len = 0;
				goto again;
			}

			continue;
		}

		break;
	}

	ret = sendall(client->fd, final_chunk, sizeof(final_chunk) - 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int dynamic_post_req(struct http_resource_detail_dynamic *dynamic_detail,
			    struct http_client_ctx *client)
{
	/* offset tells from where the POST params start */
	char *start = strstr(client->buffer, "\r\n\r\n");
	int ret, remaining, offset = 0;
	char *ptr;
	char tmp[TEMP_BUF_LEN];

	if (start == NULL) {
		return -ENOENT;
	}

	ret = sendall(client->fd, RESPONSE_TEMPLATE_CHUNKED,
		      sizeof(RESPONSE_TEMPLATE_CHUNKED) - 1);
	if (ret < 0) {
		return ret;
	}

	start += 4; /* skip \r\n\r\n */
	remaining = strlen(start);

	/* Pass URL to the client */
	while (1) {
		int copy_len, send_len;

		ptr = &start[offset];
		copy_len = MIN(remaining, dynamic_detail->data_buffer_len);

		memcpy(dynamic_detail->data_buffer, ptr, copy_len);

	again:
		send_len = dynamic_detail->cb(client, dynamic_detail->data_buffer,
					      copy_len, dynamic_detail->user_data);
		if (send_len > 0) {
			ret = snprintk(tmp, sizeof(tmp), "%x\r\n", send_len);
			ret = sendall(client->fd, tmp, ret);
			if (ret < 0) {
				return ret;
			}

			ret = sendall(client->fd, dynamic_detail->data_buffer, send_len);
			if (ret < 0) {
				return ret;
			}

			(void)sendall(client->fd, crlf, 2);

			offset += copy_len;
			remaining -= copy_len;

			/* If we have passed all the data to the application,
			 * then just pass empty buffer to it.
			 */
			if (remaining == 0) {
				copy_len = 0;
				goto again;
			}

			continue;
		}

		break;
	}

	ret = sendall(client->fd, final_chunk, sizeof(final_chunk) - 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int handle_http1_dynamic_resource(struct http_resource_detail_dynamic *dynamic_detail,
				  struct http_client_ctx *client)
{
	uint32_t user_method;
	int ret;

	if (dynamic_detail->cb == NULL) {
		return -ESRCH;
	}

	user_method = dynamic_detail->common.bitmask_of_supported_http_methods;

	if (!(BIT(client->parser.method) & user_method)) {
		return -ENOPROTOOPT;
	}

	switch (client->parser.method) {
	case HTTP_HEAD:
		if (user_method & BIT(HTTP_HEAD)) {
			ret = sendall(client->fd, RESPONSE_TEMPLATE_DYNAMIC,
				      sizeof(RESPONSE_TEMPLATE_DYNAMIC) - 1);
			if (ret < 0) {
				return ret;
			}

			return 0;
		}

	case HTTP_GET:
		/* For GET request, we do not pass any data to the app but let the app
		 * send data to the peer.
		 */
		if (user_method & BIT(HTTP_GET)) {
			return dynamic_get_req(dynamic_detail, client);
		}

		goto not_supported;

	case HTTP_POST:
		if (user_method & BIT(HTTP_POST)) {
			return dynamic_post_req(dynamic_detail, client);
		}

		goto not_supported;

	not_supported:
	default:
		LOG_DBG("HTTP method %s (%d) not supported.",
			http_method_str(client->parser.method),
			client->parser.method);

		return -ENOTSUP;
	}

	return 0;
}

int handle_http1_rest_json_resource(struct http_resource_detail_static *static_detail,
				    struct http_client_ctx *client)
{
	struct http_resource_detail_rest_json *rest_json_detail =
		CONTAINER_OF(&static_detail->common, struct http_resource_detail_rest_json, common);

	int ret, remaining, offset = rest_json_detail->common.path_len;
	char *ptr;
	char tmp[TEMP_BUF_LEN];

	if (!(static_detail->common.bitmask_of_supported_http_methods & BIT(HTTP_POST))) {
		return -ENOTSUP;
	}

	char *json_start = strstr(client->buffer, "\r\n\r\n");

	if (!json_start) {
		return -EINVAL;
	}

	ret = json_obj_parse(json_start, strlen(json_start), rest_json_detail->descr,
			     rest_json_detail->descr_len, rest_json_detail->json);
	if (ret < 0) {
		return -1;
	}

	ret = sendall(client->fd, RESPONSE_TEMPLATE_CHUNKED, sizeof(RESPONSE_TEMPLATE_CHUNKED) - 1);
	if (ret < 0) {
		return ret;
	}

	remaining = strlen(&client->url_buffer[rest_json_detail->common.path_len]);

	/* Pass URL to the client */
	while (1) {
		int copy_len, send_len;

		ptr = &client->url_buffer[offset];
		copy_len = MIN(remaining, rest_json_detail->data_buffer_len);

		memcpy(rest_json_detail->data_buffer, ptr, copy_len);

	again:

		send_len = rest_json_detail->cb(client, rest_json_detail->data_buffer, copy_len,
						rest_json_detail->user_data);
		if (send_len > 0) {
			ret = snprintk(tmp, sizeof(tmp), "%x\r\n", send_len);
			ret = sendall(client->fd, tmp, ret);
			if (ret < 0) {
				return ret;
			}

			ret = sendall(client->fd, rest_json_detail->data_buffer, send_len);
			if (ret < 0) {
				return ret;
			}

			(void)sendall(client->fd, crlf, 2);

			offset += copy_len;
			remaining -= copy_len;

			/* If we have passed all the data to the application,
			 * then just pass empty buffer to it.
			 */
			if (remaining == 0) {
				copy_len = 0;
				goto again;
			}

			continue;
		}

		break;
	}

	sendall(client->fd, final_chunk, sizeof(final_chunk) - 1);

	return 0;
}

int handle_http1_request(struct http_server_ctx *server, struct http_client_ctx *client)
{
	int total_received = 0;
	int offset = 0, ret, path_len = 0;
	struct http_resource_detail *detail;

	LOG_DBG("HTTP_SERVER_REQUEST");

	http_parser_init(&client->parser, HTTP_REQUEST);
	http_parser_settings_init(&client->parser_settings);

	client->parser_settings.on_header_field = on_header_field;
	client->parser_settings.on_url = on_url;

	http_parser_execute(&client->parser, &client->parser_settings,
			    client->buffer + offset, client->offset);

	total_received += client->offset;
	offset += client->offset;

	if (offset >= HTTP_SERVER_MAX_RESPONSE_SIZE) {
		offset = 0;
	}

	if (client->has_upgrade_header) {
		static const char response[] =
			"HTTP/1.1 101 Switching Protocols\r\n"
			"Connection: Upgrade\r\n"
			"Upgrade: h2c\r\n"
			"\r\n";

		ret = sendall(client->fd, response, sizeof(response) - 1);
		if (ret < 0) {
			close_client_connection(server, client);
			return ret;
		}

		memset(client->buffer, 0, sizeof(client->buffer));
		client->offset = 0;
		client->server_state = HTTP_SERVER_PREFACE_STATE;

		return 0;
	}

	detail = get_resource_detail(client->url_buffer, &path_len);
	if (detail != NULL) {
		detail->path_len = path_len;

		if (detail->type == HTTP_RESOURCE_TYPE_STATIC) {
			ret = handle_http1_static_resource(
				(struct http_resource_detail_static *)detail,
				client->fd);
			if (ret < 0) {
				close_client_connection(server, client);
				return ret;
			}
		} else if (detail->type == HTTP_RESOURCE_TYPE_DYNAMIC) {
			ret = handle_http1_dynamic_resource(
				(struct http_resource_detail_dynamic *)detail,
				client);
			if (ret <= 0) {
				close_client_connection(server, client);
				return ret;
			}
		} else if (detail->type == HTTP_RESOURCE_TYPE_REST_JSON) {
			handle_http1_rest_json_resource(
				(struct http_resource_detail_static *)detail, client);
		}
	} else {
		static const char not_found_response[] =
			"HTTP/1.1 404 Not Found\r\n"
			"Content-Length: 9\r\n\r\n"
			"Not Found";

		ret = sendall(client->fd, not_found_response, sizeof(not_found_response) - 1);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
		}
	}

	LOG_DBG("Connection closed client #%d", ARRAY_INDEX(server->clients, client));
	close_client_connection(server, client);

	return 0;
}

int handle_http_done(struct http_server_ctx *server, struct http_client_ctx *client)
{
	LOG_DBG("HTTP_SERVER_DONE_STATE");

	close_client_connection(server, client);

	return -errno;
}

int handle_http2_static_resource(struct http_resource_detail_static *static_detail,
				 struct http_frame *frame, int client_fd)
{
	if (static_detail->common.bitmask_of_supported_http_methods & BIT(HTTP_GET)) {
		const char *content_200 = static_detail->static_data;
		size_t content_size = static_detail->static_data_len;
		int ret;

		ret = send_headers_frame(client_fd, HTTP_SERVER_HPACK_STATUS_2OO,
					 frame->stream_identifier);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			return ret;
		}

		ret = send_data_frame(client_fd, content_200, content_size,
				      frame->stream_identifier);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			return ret;
		}
	}

	return 0;
}

int handle_http_frame_headers(struct http_client_ctx *client)
{
	struct http_frame *frame = &client->current_frame;
	struct http_resource_detail *detail;
	int bytes_consumed;
	const char *method;
	const char *path;
	int ret, path_len;

	LOG_DBG("HTTP_SERVER_FRAME_HEADERS");

	print_http_frames(client);

	if (client->offset < frame->length) {
		return -EAGAIN;
	}

	/* TODO This doesn't make much sense, the method is ignored, and
	 * handle_http2_static_resource() hardcodes it as GET.
	 */
	method = http_hpack_parse_header(client, HTTP_SERVER_HPACK_METHOD);
	path = http_hpack_parse_header(client, HTTP_SERVER_HPACK_PATH);

	detail = get_resource_detail(path, &path_len);
	if (detail != NULL) {
		detail->path_len = path_len;

		if (detail->type == HTTP_RESOURCE_TYPE_STATIC) {
			ret = handle_http2_static_resource(
				(struct http_resource_detail_static *)detail, frame,
				client->fd);
			if (ret < 0) {
				return ret;
			}
		}
	} else {
		ret = send_headers_frame(client->fd, HTTP_SERVER_HPACK_STATUS_4O4,
					 frame->stream_identifier);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			return ret;
		}

		ret = send_data_frame(client->fd, content_404, sizeof(content_404),
				      frame->stream_identifier);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			return ret;
		}
	}

	client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	bytes_consumed = client->current_frame.length;
	client->offset -= bytes_consumed;
	memmove(client->buffer, client->buffer + bytes_consumed, client->offset);

	return 0;
}

int handle_http_frame_priority(struct http_client_ctx *client)
{
	struct http_frame *frame = &client->current_frame;
	int bytes_consumed;

	LOG_DBG("HTTP_SERVER_FRAME_PRIORITY_STATE");

	print_http_frames(client);

	if (client->offset < frame->length) {
		return -EAGAIN;
	}

	bytes_consumed = client->current_frame.length;
	client->offset -= bytes_consumed;
	memmove(client->buffer, client->buffer + bytes_consumed, client->offset);

	client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	return 0;
}

int handle_http_frame_continuation(struct http_client_ctx *client)
{
	LOG_DBG("HTTP_SERVER_FRAME_CONTINUATION_STATE");
	client->server_state = HTTP_SERVER_FRAME_HEADERS_STATE;

	return 0;
}

int handle_http_frame_settings(struct http_client_ctx *client)
{
	struct http_frame *frame = &client->current_frame;
	int bytes_consumed;

	LOG_DBG("HTTP_SERVER_FRAME_SETTINGS");

	print_http_frames(client);

	if (client->offset < frame->length) {
		return -EAGAIN;
	}

	bytes_consumed = client->current_frame.length;
	client->offset -= bytes_consumed;
	memmove(client->buffer, client->buffer + bytes_consumed, client->offset);

	if (!settings_ack_flag(frame->flags)) {
		int ret;

		ret = sendall(client->fd, settings_frame, sizeof(settings_frame));
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			return ret;
		}

		ret = sendall(client->fd, settings_ack, sizeof(settings_ack));
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			return ret;
		}
	}

	client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	return 0;
}

int handle_http_frame_window_update(struct http_client_ctx *client)
{
	struct http_frame *frame = &client->current_frame;
	int bytes_consumed;

	LOG_DBG("HTTP_SERVER_FRAME_WINDOW_UPDATE");

	print_http_frames(client);

	if (client->has_upgrade_header) {
		frame->stream_identifier = 1;
		handle_http_frame_headers(client);
		client->server_state = HTTP_SERVER_FRAME_GOAWAY_STATE;

		return 0;
	}

	if (client->offset < frame->length) {
		return -EAGAIN;
	}

	bytes_consumed = client->current_frame.length;
	client->offset -= bytes_consumed;
	memmove(client->buffer, client->buffer + bytes_consumed, client->offset);

	client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	return 0;
}

int handle_http_frame_goaway(struct http_server_ctx *server, struct http_client_ctx *client)
{
	struct http_frame *frame = &client->current_frame;

	LOG_DBG("HTTP_SERVER_FRAME_GOAWAY");

	print_http_frames(client);

	if (client->offset < frame->length) {
		return -EAGAIN;
	}

	close_client_connection(server, client);

	return 0;
}

int handle_http_frame_rst_frame(struct http_server_ctx *server, struct http_client_ctx *client)
{
	struct http_frame *frame = &client->current_frame;
	int bytes_consumed;

	LOG_DBG("FRAME_RST_STREAM");

	print_http_frames(client);

	if (client->offset < frame->length) {
		return -EAGAIN;
	}

	bytes_consumed = client->current_frame.length;
	client->offset -= bytes_consumed;
	memmove(client->buffer, client->buffer + bytes_consumed, client->offset);

	client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	return 0;
}

int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
	struct http_client_ctx *ctx = CONTAINER_OF(parser,
						   struct http_client_ctx,
						   parser);

	if (length == 7 && strncasecmp(at, "Upgrade", length) == 0) {
		LOG_DBG("The \"Upgrade: h2c\" header is present.");
		ctx->has_upgrade_header = true;
	}

	return 0;
}

int on_url(struct http_parser *parser, const char *at, size_t length)
{
	struct http_client_ctx *ctx = CONTAINER_OF(parser,
						   struct http_client_ctx,
						   parser);
	strncpy(ctx->url_buffer, at, length);
	ctx->url_buffer[length] = '\0';
	LOG_DBG("Requested URL: %s", ctx->url_buffer);
	return 0;
}

int sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = zsock_send(sock, buf, len, 0);

		if (out_len < 0) {
			return -errno;
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
	int ret;

	encode_frame_header(frame_header, sizeof(headers_payload),
			    HTTP_SERVER_HEADERS_FRAME,
			    HTTP_SERVER_FLAG_END_HEADERS, stream_id);

	ret = sendall(socket_fd, frame_header, sizeof(frame_header));
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
		return ret;
	}

	ret = sendall(socket_fd, headers_payload, sizeof(headers_payload));
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
		return ret;
	}

	return 0;
}

int send_data_frame(int socket_fd, const char *payload, size_t length, uint32_t stream_id)
{
	uint8_t frame_header[HTTP_SERVER_FRAME_HEADER_SIZE];
	int ret;

	encode_frame_header(frame_header, length, HTTP_SERVER_DATA_FRAME,
			    HTTP_SERVER_FLAG_END_STREAM, stream_id);

	ret = sendall(socket_fd, frame_header, sizeof(frame_header));
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
		return ret;
	}

	ret = sendall(socket_fd, payload, length);
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
		return ret;
	}

	return 0;
}

void print_http_frames(struct http_client_ctx *client)
{
#if defined(PRINT_COLOR)
	const char *bold = "\033[1m";
	const char *reset = "\033[0m";
	const char *green = "\033[32m";
	const char *blue = "\033[34m";
#else
	const char *bold = "";
	const char *reset = "";
	const char *green = "";
	const char *blue = "";
#endif

	struct http_frame *frame = &client->current_frame;
	int payload_received_length;

	LOG_DBG("%s=====================================%s", green, reset);
	LOG_DBG("%sReceived %s Frame :%s", bold, get_frame_type_name(frame->type), reset);
	LOG_DBG("  %sLength:%s %u", blue, reset, frame->length);
	LOG_DBG("  %sType:%s %u (%s)", blue, reset, frame->type, get_frame_type_name(frame->type));
	LOG_DBG("  %sFlags:%s %u", blue, reset, frame->flags);
	LOG_DBG("  %sStream Identifier:%s %u", blue, reset, frame->stream_identifier);

	if (client->offset > frame->length) {
		payload_received_length = frame->length;
	} else {
		payload_received_length = client->offset;
	}

	LOG_HEXDUMP_DBG(frame->payload, payload_received_length, "Payload");
	LOG_DBG("%s=====================================%s", green, reset);
}

int parse_http_frame_header(struct http_client_ctx *client)
{
	unsigned char *buffer = client->buffer;
	unsigned long buffer_len = client->offset;
	struct http_frame *frame = &client->current_frame;

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

	LOG_DBG("Frame len %d type 0x%02x flags 0x%02x id %d",
		frame->length, frame->type, frame->flags, frame->stream_identifier);

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
