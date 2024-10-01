/*
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/posix/sys/eventfd.h>
#include <zephyr/posix/fnmatch.h>

LOG_MODULE_REGISTER(net_http_server, CONFIG_NET_HTTP_SERVER_LOG_LEVEL);

#include "../../ip/net_private.h"
#include "headers/server_internal.h"

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
/* Lowest priority cooperative thread */
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#endif

#define INVALID_SOCK -1
#define INACTIVITY_TIMEOUT K_SECONDS(CONFIG_HTTP_SERVER_CLIENT_INACTIVITY_TIMEOUT)

#define HTTP_SERVER_MAX_SERVICES CONFIG_HTTP_SERVER_NUM_SERVICES
#define HTTP_SERVER_MAX_CLIENTS  CONFIG_HTTP_SERVER_MAX_CLIENTS
#define HTTP_SERVER_SOCK_COUNT (1 + HTTP_SERVER_MAX_SERVICES + HTTP_SERVER_MAX_CLIENTS)

struct http_server_ctx {
	int num_clients;
	int listen_fds; /* max value of 1 + MAX_SERVICES */

	/* First pollfd is eventfd that can be used to stop the server,
	 * then we have the server listen sockets,
	 * and then the accepted sockets.
	 */
	struct zsock_pollfd fds[HTTP_SERVER_SOCK_COUNT];
	struct http_client_ctx clients[HTTP_SERVER_MAX_CLIENTS];
};

static struct http_server_ctx server_ctx;
static K_SEM_DEFINE(server_start, 0, 1);
static bool server_running;

static void close_client_connection(struct http_client_ctx *client);

HTTP_SERVER_CONTENT_TYPE(html, "text/html")
HTTP_SERVER_CONTENT_TYPE(css, "text/css")
HTTP_SERVER_CONTENT_TYPE(js, "text/javascript")
HTTP_SERVER_CONTENT_TYPE(jpg, "image/jpeg")
HTTP_SERVER_CONTENT_TYPE(png, "image/png")
HTTP_SERVER_CONTENT_TYPE(svg, "image/svg+xml")

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
	ctx->fds[count].events = ZSOCK_POLLIN;
	count++;

	HTTP_SERVICE_FOREACH(svc) {
		/* set the default address (in6addr_any / INADDR_ANY are all 0) */
		memset(&addr_storage, 0, sizeof(struct sockaddr_storage));

		/* Set up the server address struct according to address family */
		if (IS_ENABLED(CONFIG_NET_IPV6) && svc->host != NULL &&
		    zsock_inet_pton(AF_INET6, svc->host, &addr.addr6->sin6_addr) == 1) {
			af = AF_INET6;
			len = sizeof(*addr.addr6);

			addr.addr6->sin6_family = AF_INET6;
			addr.addr6->sin6_port = htons(*svc->port);
		} else if (IS_ENABLED(CONFIG_NET_IPV4) && svc->host != NULL &&
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

		/* If IPv4-to-IPv6 mapping is enabled, then turn off V6ONLY option
		 * so that IPv6 socket can serve IPv4 connections.
		 */
		if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6)) {
			int optval = 0;

			(void)setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY,
					 &optval, sizeof(optval));
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

		if (zsock_listen(fd, HTTP_SERVER_MAX_CLIENTS) < 0) {
			LOG_ERR("listen: %d", errno);
			failed++;
			zsock_close(fd);
			continue;
		}

		LOG_DBG("Initialized HTTP Service %s:%u",
			svc->host ? svc->host : "<any>", *svc->port);

		ctx->fds[count].fd = fd;
		ctx->fds[count].events = ZSOCK_POLLIN;
		count++;
	}

	if (failed >= svc_count) {
		LOG_ERR("All services failed (%d)", failed);
		/* Close eventfd socket */
		zsock_close(ctx->fds[0].fd);
		return -ESRCH;
	}

	ctx->listen_fds = count;
	ctx->num_clients = 0;

	return 0;
}

static int accept_new_client(int server_fd)
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

static void close_all_sockets(struct http_server_ctx *ctx)
{
	zsock_close(ctx->fds[0].fd); /* close eventfd */
	ctx->fds[0].fd = -1;

	for (int i = 1; i < ARRAY_SIZE(ctx->fds); i++) {
		if (ctx->fds[i].fd < 0) {
			continue;
		}

		if (i < ctx->listen_fds) {
			zsock_close(ctx->fds[i].fd);
		} else {
			struct http_client_ctx *client =
				&server_ctx.clients[i - ctx->listen_fds];

			close_client_connection(client);
		}

		ctx->fds[i].fd = -1;
	}
}

static void client_release_resources(struct http_client_ctx *client)
{
	struct http_resource_detail *detail;
	struct http_resource_detail_dynamic *dynamic_detail;
	struct http_response_ctx response_ctx;

	HTTP_SERVICE_FOREACH(service) {
		HTTP_SERVICE_FOREACH_RESOURCE(service, resource) {
			detail = resource->detail;

			if (detail->type != HTTP_RESOURCE_TYPE_DYNAMIC) {
				continue;
			}

			dynamic_detail = (struct http_resource_detail_dynamic *)detail;

			if (dynamic_detail->holder != client) {
				continue;
			}

			/* If the client still holds the resource at this point,
			 * it means the transaction was not complete. Release
			 * the resource and notify application.
			 */
			dynamic_detail->holder = NULL;

			if (dynamic_detail->cb == NULL) {
				continue;
			}

			dynamic_detail->cb(client, HTTP_SERVER_DATA_ABORTED, NULL, 0, &response_ctx,
					   dynamic_detail->user_data);
		}
	}
}

void http_server_release_client(struct http_client_ctx *client)
{
	int i;
	struct k_work_sync sync;

	__ASSERT_NO_MSG(IS_ARRAY_ELEMENT(server_ctx.clients, client));

	k_work_cancel_delayable_sync(&client->inactivity_timer, &sync);
	client_release_resources(client);

	server_ctx.num_clients--;

	for (i = server_ctx.listen_fds; i < ARRAY_SIZE(server_ctx.fds); i++) {
		if (server_ctx.fds[i].fd == client->fd) {
			server_ctx.fds[i].fd = INVALID_SOCK;
			break;
		}
	}

	memset(client, 0, sizeof(struct http_client_ctx));
	client->fd = INVALID_SOCK;
}

static void close_client_connection(struct http_client_ctx *client)
{
	int fd = client->fd;

	http_server_release_client(client);

	(void)zsock_close(fd);
}

static void client_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct http_client_ctx *client =
		CONTAINER_OF(dwork, struct http_client_ctx, inactivity_timer);

	LOG_DBG("Client %p timeout", client);

	/* Shutdown the socket. This will be detected by poll() and a proper
	 * cleanup will proceed.
	 */
	(void)zsock_shutdown(client->fd, ZSOCK_SHUT_RD);
}

void http_client_timer_restart(struct http_client_ctx *client)
{
	__ASSERT_NO_MSG(IS_ARRAY_ELEMENT(server_ctx.clients, client));

	k_work_reschedule(&client->inactivity_timer, INACTIVITY_TIMEOUT);
}

static void init_client_ctx(struct http_client_ctx *client, int new_socket)
{
	client->fd = new_socket;
	client->data_len = 0;
	client->server_state = HTTP_SERVER_PREFACE_STATE;
	client->has_upgrade_header = false;
	client->preface_sent = false;
	client->window_size = HTTP_SERVER_INITIAL_WINDOW_SIZE;

	memset(client->buffer, 0, sizeof(client->buffer));
	memset(client->url_buffer, 0, sizeof(client->url_buffer));
	k_work_init_delayable(&client->inactivity_timer, client_timeout);
	http_client_timer_restart(client);

	ARRAY_FOR_EACH(client->streams, i) {
		client->streams[i].stream_state = HTTP2_STREAM_IDLE;
		client->streams[i].stream_id = 0;
	}

	client->current_stream = NULL;
}

static int handle_http_preface(struct http_client_ctx *client)
{
	LOG_DBG("HTTP_SERVER_PREFACE_STATE.");

	if (client->data_len < sizeof(HTTP2_PREFACE) - 1) {
		/* We don't have full preface yet, get more data. */
		return -EAGAIN;
	}

#if defined(CONFIG_HTTP_SERVER_CAPTURE_HEADERS)
	client->header_capture_ctx.count = 0;
	client->header_capture_ctx.cursor = 0;
	client->header_capture_ctx.status = HTTP_HEADER_STATUS_OK;
#endif /* defined(CONFIG_HTTP_SERVER_CAPTURE_HEADERS) */

	if (strncmp(client->cursor, HTTP2_PREFACE, sizeof(HTTP2_PREFACE) - 1) != 0) {
		return enter_http1_request(client);
	}

	return enter_http2_request(client);
}

static int handle_http_done(struct http_client_ctx *client)
{
	LOG_DBG("HTTP_SERVER_DONE_STATE");

	close_client_connection(client);

	return -EAGAIN;
}

int enter_http_done_state(struct http_client_ctx *client)
{
	close_client_connection(client);

	client->server_state = HTTP_SERVER_DONE_STATE;

	return -EAGAIN;
}

static int handle_http_request(struct http_client_ctx *client)
{
	int ret = -EINVAL;

	client->cursor = client->buffer;

	do {
		switch (client->server_state) {
		case HTTP_SERVER_FRAME_DATA_STATE:
			ret = handle_http_frame_data(client);
			break;
		case HTTP_SERVER_PREFACE_STATE:
			ret = handle_http_preface(client);
			break;
		case HTTP_SERVER_REQUEST_STATE:
			ret = handle_http1_request(client);
			break;
		case HTTP_SERVER_FRAME_HEADER_STATE:
			ret = handle_http_frame_header(client);
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
			ret = handle_http_frame_rst_stream(client);
			break;
		case HTTP_SERVER_FRAME_GOAWAY_STATE:
			ret = handle_http_frame_goaway(client);
			break;
		case HTTP_SERVER_FRAME_PRIORITY_STATE:
			ret = handle_http_frame_priority(client);
			break;
		case HTTP_SERVER_FRAME_PADDING_STATE:
			ret = handle_http_frame_padding(client);
			break;
		case HTTP_SERVER_DONE_STATE:
			ret = handle_http_done(client);
			break;
		default:
			ret = handle_http_done(client);
			break;
		}
	} while (ret >= 0 && client->data_len > 0);

	if (ret < 0 && ret != -EAGAIN) {
		return ret;
	}

	if (client->data_len > 0) {
		/* Move any remaining data in the buffer. */
		memmove(client->buffer, client->cursor, client->data_len);
	}

	return 0;
}

static int http_server_run(struct http_server_ctx *ctx)
{
	struct http_client_ctx *client;
	eventfd_t value;
	bool found_slot;
	int new_socket;
	int ret, i, j;
	int sock_error;
	socklen_t optlen = sizeof(int);

	value = 0;

	while (1) {
		ret = zsock_poll(ctx->fds, HTTP_SERVER_SOCK_COUNT, -1);
		if (ret < 0) {
			ret = -errno;
			LOG_DBG("poll failed (%d)", ret);
			goto closing;
		}

		if (ret == 0) {
			/* should not happen because timeout is -1 */
			break;
		}

		if (ret == 1 && ctx->fds[0].revents) {
			eventfd_read(ctx->fds[0].fd, &value);
			LOG_DBG("Received stop event. exiting ..");
			ret = 0;
			goto closing;
		}

		for (i = 1; i < ARRAY_SIZE(ctx->fds); i++) {
			if (ctx->fds[i].fd < 0) {
				continue;
			}

			if (ctx->fds[i].revents & ZSOCK_POLLHUP) {
				if (i >= ctx->listen_fds) {
					LOG_DBG("Client #%d has disconnected",
						i - ctx->listen_fds);

					client = &ctx->clients[i - ctx->listen_fds];
					close_client_connection(client);
				}

				continue;
			}

			if (ctx->fds[i].revents & ZSOCK_POLLERR) {
				(void)zsock_getsockopt(ctx->fds[i].fd, SOL_SOCKET,
						       SO_ERROR, &sock_error, &optlen);
				LOG_DBG("Error on fd %d %d", ctx->fds[i].fd, sock_error);

				if (i >= ctx->listen_fds) {
					client = &ctx->clients[i - ctx->listen_fds];
					close_client_connection(client);
					continue;
				}

				/* Listening socket error, abort. */
				LOG_ERR("Listening socket error, aborting.");
				ret = -sock_error;
				goto closing;

			}

			if (!(ctx->fds[i].revents & ZSOCK_POLLIN)) {
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
					ctx->fds[j].events = ZSOCK_POLLIN;
					ctx->fds[j].revents = 0;

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

				continue;
			}

			/* Client sock */
			client = &ctx->clients[i - ctx->listen_fds];

			ret = zsock_recv(client->fd, client->buffer + client->data_len,
					 sizeof(client->buffer) - client->data_len, 0);
			if (ret <= 0) {
				if (ret == 0) {
					LOG_DBG("Connection closed by peer for client #%d",
						i - ctx->listen_fds);
				} else {
					ret = -errno;
					LOG_DBG("ERROR reading from socket (%d)", ret);
				}

				close_client_connection(client);
				continue;
			}

			client->data_len += ret;

			http_client_timer_restart(client);

			ret = handle_http_request(client);
			if (ret < 0 && ret != -EAGAIN) {
				if (ret == -ENOTCONN) {
					LOG_DBG("Client closed connection while handling request");
				} else {
					LOG_ERR("HTTP request handling error (%d)", ret);
				}
				close_client_connection(client);
			} else if (client->data_len == sizeof(client->buffer)) {
				/* If the RX buffer is still full after parsing,
				 * it means we won't be able to handle this request
				 * with the current buffer size.
				 */
				LOG_ERR("RX buffer too small to handle request");
				close_client_connection(client);
			}
		}
	}

	return 0;

closing:
	/* Close all client connections and the server socket */
	close_all_sockets(ctx);
	return ret;
}

/* Compare two strings where the terminator is either "\0" or "?" */
static int compare_strings(const char *s1, const char *s2)
{
	while ((*s1 && *s2) && (*s1 == *s2) && (*s1 != '?')) {
		s1++;
		s2++;
	}

	/* Check if both strings have reached their terminators or '?' */
	if ((*s1 == '\0' || *s1 == '?') && (*s2 == '\0' || *s2 == '?')) {
		return 0; /* Strings are equal */
	}

	return 1; /* Strings are not equal */
}

static bool skip_this(struct http_resource_desc *resource, bool is_websocket)
{
	struct http_resource_detail *detail;

	detail = (struct http_resource_detail *)resource->detail;

	if (is_websocket) {
		if (detail->type != HTTP_RESOURCE_TYPE_WEBSOCKET) {
			return true;
		}
	} else {
		if (detail->type == HTTP_RESOURCE_TYPE_WEBSOCKET) {
			return true;
		}
	}

	return false;
}

struct http_resource_detail *get_resource_detail(const char *path,
						 int *path_len,
						 bool is_websocket)
{
	HTTP_SERVICE_FOREACH(service) {
		HTTP_SERVICE_FOREACH_RESOURCE(service, resource) {
			if (skip_this(resource, is_websocket)) {
				continue;
			}

			if (IS_ENABLED(CONFIG_HTTP_SERVER_RESOURCE_WILDCARD)) {
				int ret;

				ret = fnmatch(resource->resource, path,
					      (FNM_PATHNAME | FNM_LEADING_DIR));
				if (ret == 0) {
					*path_len = strlen(resource->resource);
					return resource->detail;
				}
			}

			if (compare_strings(path, resource->resource) == 0) {
				NET_DBG("Got match for %s", resource->resource);

				*path_len = strlen(resource->resource);
				return resource->detail;
			}
		}
	}

	NET_DBG("No match for %s", path);

	return NULL;
}

int http_server_find_file(char *fname, size_t fname_size, size_t *file_size, bool *gzipped)
{
	struct fs_dirent dirent;
	size_t len;
	int ret;

	ret = fs_stat(fname, &dirent);
	if (ret < 0) {
		len = strlen(fname);
		snprintk(fname + len, fname_size - len, ".gz");
		ret = fs_stat(fname, &dirent);
		*gzipped = (ret == 0);
	}

	if (ret == 0) {
		*file_size = dirent.size;
		return ret;
	}

	return -ENOENT;
}

void http_server_get_content_type_from_extension(char *url, char *content_type,
						 size_t content_type_size)
{
	size_t url_len = strlen(url);

	HTTP_SERVER_CONTENT_TYPE_FOREACH(ct) {
		char *ext = &url[url_len - ct->extension_len];

		if (strncmp(ext, ct->extension, ct->extension_len) == 0) {
			strncpy(content_type, ct->content_type, content_type_size);
			return;
		}
	}
}

int http_server_sendall(struct http_client_ctx *client, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = zsock_send(client->fd, buf, len, 0);

		if (out_len < 0) {
			return -errno;
		}

		buf = (const char *)buf + out_len;
		len -= out_len;

		http_client_timer_restart(client);
	}

	return 0;
}

bool http_response_is_final(struct http_response_ctx *rsp, enum http_data_status status)
{
	if (status != HTTP_SERVER_DATA_FINAL) {
		return false;
	}

	if (rsp->final_chunk ||
	    (rsp->status == 0 && rsp->header_count == 0 && rsp->body_len == 0)) {
		return true;
	}

	return false;
}

bool http_response_is_provided(struct http_response_ctx *rsp)
{
	if (rsp->status != 0 || rsp->header_count > 0 || rsp->body_len > 0) {
		return true;
	}

	return false;
}

int http_server_start(void)
{
	if (server_running) {
		LOG_DBG("HTTP server already started");
		return -EALREADY;
	}

	server_running = true;
	k_sem_give(&server_start);

	LOG_DBG("Starting HTTP server");

	return 0;
}

int http_server_stop(void)
{
	if (!server_running) {
		LOG_DBG("HTTP server already stopped");
		return -EALREADY;
	}

	server_running = false;
	k_sem_reset(&server_start);
	eventfd_write(server_ctx.fds[0].fd, 1);

	LOG_DBG("Stopping HTTP server");

	return 0;
}

static void http_server_thread(void *p1, void *p2, void *p3)
{
	int ret;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&server_start, K_FOREVER);

		while (server_running) {
			ret = http_server_init(&server_ctx);
			if (ret < 0) {
				LOG_ERR("Failed to initialize HTTP2 server");
				goto again;
			}

			ret = http_server_run(&server_ctx);
			if (!server_running) {
				continue;
			}

again:
			LOG_INF("Re-starting server (%d)", ret);
			k_sleep(K_MSEC(CONFIG_HTTP_SERVER_RESTART_DELAY));
		}
	}
}

K_THREAD_DEFINE(http_server_tid, CONFIG_HTTP_SERVER_STACK_SIZE,
		http_server_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
