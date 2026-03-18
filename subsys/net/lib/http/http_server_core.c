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

#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_interface.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/quic.h>
#include <zephyr/zvfs/eventfd.h>
#include <zephyr/posix/fnmatch.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(net_http_server, CONFIG_NET_HTTP_SERVER_LOG_LEVEL);

#include "../../ip/net_private.h"
#include "headers/server_internal.h"

BUILD_ASSERT(CONFIG_HTTP_SERVER_VERSION > 0,
	     "HTTP server requires at least HTTP/1.x, HTTP/2 or HTTP/3 support");

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
/* Lowest priority cooperative thread */
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#endif

#define INACTIVITY_TIMEOUT K_SECONDS(CONFIG_HTTP_SERVER_CLIENT_INACTIVITY_TIMEOUT)

#define HTTP_SERVER_MAX_SERVICES CONFIG_HTTP_SERVER_NUM_SERVICES
#define HTTP_SERVER_MAX_CLIENTS  CONFIG_HTTP_SERVER_MAX_CLIENTS
#define HTTP_SERVER_SOCK_COUNT (1 + HTTP_SERVER_MAX_SERVICES + \
				HTTP_SERVER_MAX_CLIENTS  + HTTP3_SERVER_MAX_STREAMS)
struct http_server_ctx {
	int listen_fds; /* max value of 1 + MAX_SERVICES */
	int client_fds; /* max value of 1 + MAX_CLIENTS */

	/* First pollfd is eventfd that can be used to stop the server,
	 * then we have the server listen sockets,
	 * and then the accepted sockets.
	 * If HTTP/3 is enabled, the rest are for accepted streams.
	 */
	struct zsock_pollfd fds[HTTP_SERVER_SOCK_COUNT];
	struct http_client_ctx clients[HTTP_SERVER_MAX_CLIENTS];
};

static struct http_server_ctx server_ctx;
static K_SEM_DEFINE(server_start, 0, 1);
static bool server_running;

#if defined(CONFIG_HTTP_SERVER_TLS_USE_ALPN)
#if defined(CONFIG_HTTP_SERVER_VERSION_1) || defined(CONFIG_HTTP_SERVER_VERSION_2)
static const char *const h1_h2_alpn_list[] = {
	IF_ENABLED(CONFIG_HTTP_SERVER_VERSION_2, ("h2",))
	IF_ENABLED(CONFIG_HTTP_SERVER_VERSION_1, ("http/1.1",))
};
#endif

#if defined(CONFIG_HTTP_SERVER_VERSION_3)
static const char *const h3_alpn_list[] = {
	"h3",
};
#endif
#endif

static void close_client_connection(struct http_client_ctx *client);

HTTP_SERVER_CONTENT_TYPE(html, "text/html")
HTTP_SERVER_CONTENT_TYPE(css, "text/css")
HTTP_SERVER_CONTENT_TYPE(js, "text/javascript")
HTTP_SERVER_CONTENT_TYPE(jpg, "image/jpeg")
HTTP_SERVER_CONTENT_TYPE(png, "image/png")
HTTP_SERVER_CONTENT_TYPE(svg, "image/svg+xml")


static int setup_h1_h2_socket(const struct http_service_desc *svc, int af,
			      struct net_sockaddr *addr, net_socklen_t len)
{
	int proto;
	int fd;

	/* Create a socket */
	if (COND_CODE_1(CONFIG_NET_SOCKETS_SOCKOPT_TLS,
			(svc->sec_tag_list != NULL),
			(0))) {
		proto = NET_IPPROTO_TLS_1_2;
	} else {
		proto = NET_IPPROTO_TCP;
	}

	if (svc->config != NULL && svc->config->socket_create != NULL) {
		fd = svc->config->socket_create(svc, af, proto);
	} else {
		fd = zsock_socket(af, NET_SOCK_STREAM, proto);
	}

	if (fd < 0) {
		fd = -errno;
		LOG_ERR("socket: %d", fd);
		return fd;
	}

	/* If IPv4-to-IPv6 mapping is enabled, then turn off V6ONLY option
	 * so that IPv6 socket can serve IPv4 connections.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6)) {
		int optval = 0;

		(void)zsock_setsockopt(fd, NET_IPPROTO_IPV6, ZSOCK_IPV6_V6ONLY, &optval,
				       sizeof(optval));
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	if (svc->sec_tag_list != NULL) {
		if (zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
				     svc->sec_tag_list,
				     svc->sec_tag_list_size) < 0) {
			LOG_ERR("%s: setsockopt(%s): %d", "h1/2", "TLS_SEC_TAG_LIST", errno);
			zsock_close(fd);
			return -errno;
		}

#if defined(CONFIG_HTTP_SERVER_TLS_USE_ALPN)
#if defined(CONFIG_HTTP_SERVER_VERSION_1) || defined(CONFIG_HTTP_SERVER_VERSION_2)
		if (zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_ALPN_LIST,
				     h1_h2_alpn_list,
				     sizeof(h1_h2_alpn_list)) < 0) {
			LOG_ERR("%s: setsockopt(%s): %d", "h1/2", "TLS_ALPN_LIST", errno);
			zsock_close(fd);
			return -errno;
		}
#endif
#endif /* defined(CONFIG_HTTP_SERVER_TLS_USE_ALPN) */
	}
#endif /* defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) */

	if (zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &(int){1},
			     sizeof(int)) < 0) {
		LOG_ERR("%s: setsockopt(%s): %d", "h1/2", "SO_REUSEADDR", errno);
		zsock_close(fd);
		return -errno;
	}

	if (zsock_bind(fd, addr, len) < 0) {
		LOG_ERR("bind: %d", errno);
		zsock_close(fd);
		return -errno;
	}

	if (*svc->port == 0) {
		/* Ephemeral port, read back the port number */
		net_socklen_t slen = sizeof(struct net_sockaddr_storage);

		if (zsock_getsockname(fd, addr, &slen) < 0) {
			LOG_ERR("getsockname: %d", errno);
			zsock_close(fd);
			return -errno;
		}

		*svc->port = net_ntohs(net_sin(addr)->sin_port);
	}

	svc->data->num_clients = 0;
	if (zsock_listen(fd, svc->backlog) < 0) {
		LOG_ERR("listen: %d", errno);
		zsock_close(fd);
		return -errno;
	}

	return fd;
}

static int setup_h3_socket(const struct http_service_desc *svc, int af,
			   struct net_sockaddr *addr, net_socklen_t len)
{
	int quic_sock;
	int ret;

	if (net_sin(addr)->sin_port == 0) {
		NET_ERR("No local port specified for QUIC service");
		return -EINVAL;
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	if (svc->sec_tag_list == NULL) {
		NET_ERR("QUIC service requires TLS credentials, "
			"skipping HTTP/3 support for port %d",
			net_ntohs(net_sin(addr)->sin_port));
		return -EINVAL;
	}
#endif

	quic_sock = quic_connection_open(NULL, addr);
	if (quic_sock < 0) {
		NET_ERR("Failed to open QUIC connection socket (%d)", quic_sock);
		ret = quic_sock;
		goto out;
	}

	NET_INFO("QUIC %s local connection socket %d opened successfully",
		 addr->sa_family == NET_AF_INET6 ? "IPv6" : "IPv4",
		 quic_sock);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	if (zsock_setsockopt(quic_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
			     svc->sec_tag_list,
			     svc->sec_tag_list_size) < 0) {
		ret = -errno;
		LOG_ERR("%s: setsockopt(%s): %d", "h3", "TLS_SEC_TAG_LIST", ret);
		zsock_close(quic_sock);
		goto out;
	}

#if defined(CONFIG_HTTP_SERVER_TLS_USE_ALPN)
		if (zsock_setsockopt(quic_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_ALPN_LIST,
				     h3_alpn_list, sizeof(h3_alpn_list)) < 0) {
			ret = -errno;
			LOG_ERR("%s: setsockopt(%s): %d", "h3", "TLS_ALPN_LIST", ret);
			zsock_close(quic_sock);
			goto out;
	}
#endif /* defined(CONFIG_HTTP_SERVER_TLS_USE_ALPN) */
#endif /* defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) */

	ret = quic_sock;
out:
	return ret;
}

int http_server_init(struct http_server_ctx *ctx)
{
	int failed = 0, count = 0;
	int svc_count;
	net_socklen_t len;
	int fd, af, i;
	bool h3 = false, h2 = false, h1 = false;
	struct net_sockaddr_storage addr_storage;
	const union {
		struct net_sockaddr *addr;
		struct net_sockaddr_in *addr4;
		struct net_sockaddr_in6 *addr6;
	} addr = {
		.addr = (struct net_sockaddr *)&addr_storage
	};

	HTTP_SERVICE_COUNT(&svc_count);

	if (svc_count > HTTP_SERVER_MAX_SERVICES) {
		LOG_ERR("Found %d HTTP services, but "
			"CONFIG_HTTP_SERVER_NUM_SERVICES only allows %d",
			svc_count, HTTP_SERVER_MAX_SERVICES);
		return -EINVAL;
	}

	/* Initialize fds */
	memset(ctx->fds, 0, sizeof(ctx->fds));
	memset(ctx->clients, 0, sizeof(ctx->clients));

	for (i = 0; i < ARRAY_SIZE(ctx->fds); i++) {
		ctx->fds[i].fd = INVALID_SOCK;
	}

	/* Create an eventfd that can be used to trigger events during polling */
	fd = zvfs_eventfd(0, 0);
	if (fd < 0) {
		fd = -errno;
		LOG_ERR("eventfd failed (%d)", fd);
		return fd;
	}

	ctx->fds[count].fd = fd;
	ctx->fds[count].events = ZSOCK_POLLIN;
	count++;

	HTTP_SERVICE_FOREACH(svc) {
		/* set the default address (in6addr_any / NET_INADDR_ANY are all 0) */
		memset(&addr_storage, 0, sizeof(struct net_sockaddr_storage));

		/* Set up the server address struct according to address family */
		if (IS_ENABLED(CONFIG_NET_IPV6) && svc->host != NULL &&
		    zsock_inet_pton(NET_AF_INET6, svc->host, &addr.addr6->sin6_addr) == 1) {
			af = NET_AF_INET6;
			len = sizeof(*addr.addr6);

			addr.addr6->sin6_family = NET_AF_INET6;
			addr.addr6->sin6_port = net_htons(*svc->port);
		} else if (IS_ENABLED(CONFIG_NET_IPV4) && svc->host != NULL &&
			   zsock_inet_pton(NET_AF_INET, svc->host, &addr.addr4->sin_addr) == 1) {
			af = NET_AF_INET;
			len = sizeof(*addr.addr4);

			addr.addr4->sin_family = NET_AF_INET;
			addr.addr4->sin_port = net_htons(*svc->port);
		} else if (IS_ENABLED(CONFIG_NET_IPV6)) {
			/* prefer IPv6 if both IPv6 and IPv4 are supported */
			af = NET_AF_INET6;
			len = sizeof(*addr.addr6);

			addr.addr6->sin6_family = NET_AF_INET6;
			addr.addr6->sin6_port = net_htons(*svc->port);
		} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
			af = NET_AF_INET;
			len = sizeof(*addr.addr4);

			addr.addr4->sin_family = NET_AF_INET;
			addr.addr4->sin_port = net_htons(*svc->port);
		} else {
			LOG_ERR("Neither IPv4 nor IPv6 is enabled");
			failed++;
			break;
		}

		/* Allow application to specify which HTTP versions to support
		 * at runtime. If not specified, support all enabled versions.
		 */
		if (svc->config != NULL) {
			if (svc->config->http_ver & HTTP_VERSION_3) {
				if (!IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3)) {
					NET_WARN("HTTP/%d is not enabled but service requires it!",
						 3);
				} else {
					h3 = true;
				}
			}

			if (svc->config->http_ver & HTTP_VERSION_2) {
				if (!IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2)) {
					NET_WARN("HTTP/%d is not enabled but service requires it!",
						 2);
				} else {
					h2 = true;
				}
			}

			if (svc->config->http_ver & HTTP_VERSION_1) {
				if (!IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_1)) {
					NET_WARN("HTTP/%d is not enabled but service requires it!",
						 1);
				} else {
					h1 = true;
				}
			}

			if (svc->config->http_ver == HTTP_VERSION_ANY) {
				h1 = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_1);
				h2 = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2);
				h3 = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3);
			}
		} else {
			h1 = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_1);
			h2 = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2);
			h3 = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3);
		}

		if (h1 || h2) {
			fd = setup_h1_h2_socket(svc, af, addr.addr, len);
			if (fd < 0) {
				failed++;
			} else {
				if (IS_ENABLED(CONFIG_NET_HTTP_SERVER_LOG_LEVEL_DBG)) {
					char ver_str[sizeof("/1.1/2")] = {0};

					if (h1) {
						strcat(ver_str, "/1.1");
					}

					if (h2) {
						strcat(ver_str, "/2");
					}

					LOG_DBG("Initialized HTTP%s Service %s:%u",
						ver_str,
						svc->host ? svc->host : "<any>",
						*svc->port);
				}

				*svc->fd = fd;
				ctx->fds[count].fd = fd;
				ctx->fds[count].events = ZSOCK_POLLIN;
				count++;
			}
		}

		if (h3 && IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3)) {
			fd = setup_h3_socket(svc, af, addr.addr, len);
			if (fd < 0) {
				failed++;
			} else {
				LOG_DBG("Initialized HTTP%s Service %s:%u", "/3",
					svc->host ? svc->host : "<any>", *svc->port);

				*svc->fd_h3 = fd;
				ctx->fds[count].fd = fd;
				ctx->fds[count].events = ZSOCK_POLLIN;
				count++;
			}
		}
	}

	if (failed >= svc_count) {
		LOG_ERR("All services failed (%d)", failed);
		/* Close eventfd socket */
		zsock_close(ctx->fds[0].fd);
		return -ESRCH;
	}

	ctx->listen_fds = count;
	ctx->client_fds = ctx->listen_fds + HTTP_SERVER_MAX_CLIENTS;

	if (!IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3) &&
	    ctx->client_fds > ARRAY_SIZE(ctx->fds)) {
		LOG_ERR("HTTP server fd table size mismatch: %d slots required, %zu allocated",
			ctx->client_fds, ARRAY_SIZE(ctx->fds));
	}

	return 0;
}

static int accept_new_client(int server_fd)
{
	int new_socket;
	net_socklen_t addrlen;
	struct net_sockaddr_storage sa;

	memset(&sa, 0, sizeof(sa));
	addrlen = sizeof(sa);

	new_socket = zsock_accept(server_fd, (struct net_sockaddr *)&sa, &addrlen);
	if (new_socket < 0) {
		new_socket = -errno;
		LOG_DBG("[%d] accept failed (%d)", server_fd, new_socket);
		return new_socket;
	}

	const char * const addrstr =
		net_sprint_addr(sa.ss_family, &net_sin((struct net_sockaddr *)&sa)->sin_addr);
	LOG_DBG("New client from %s:%d", addrstr != NULL ? addrstr : "<unknown>",
		net_ntohs(net_sin((struct net_sockaddr *)&sa)->sin_port));

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
		} else if (i < ctx->client_fds) {
			struct http_client_ctx *client =
				&server_ctx.clients[i - ctx->listen_fds];

			close_client_connection(client);
		} else {
			/* HTTP/3 stream sockets */
			zsock_close(ctx->fds[i].fd);
		}

		ctx->fds[i].fd = -1;
	}

	HTTP_SERVICE_FOREACH(svc) {
		*svc->fd = -1;

		if (IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3)) {
			*svc->fd_h3 = -1;
		}
	}
}

static void client_release_resources(struct http_client_ctx *client)
{
	struct http_resource_detail *detail;
	struct http_resource_detail_dynamic *dynamic_detail;
	struct http_request_ctx request_ctx;
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

			populate_request_ctx(&request_ctx, NULL, 0, NULL);

			dynamic_detail->cb(client, HTTP_SERVER_TRANSACTION_ABORTED, &request_ctx,
					   &response_ctx, dynamic_detail->user_data);
		}
	}
}

void http_server_release_client(struct http_client_ctx *client)
{
	int i;

	__ASSERT_NO_MSG(IS_ARRAY_ELEMENT(server_ctx.clients, client));

	/*
	 * Use the non-blocking cancel. The _sync variant deadlocks when
	 * called from within the work handler (client_timeout) because it
	 * waits for the work item to finish executing, which is us.
	 * The non-blocking cancel is sufficient: if the timer is pending it
	 * gets cancelled; if it is currently running (we are inside it) the
	 * cancel is a no-op, which is fine because the handler will not
	 * reschedule itself.
	 */
	(void)k_work_cancel_delayable(&client->inactivity_timer);

	client_release_resources(client);

	client->service->data->num_clients--;

	for (i = 0; i < server_ctx.listen_fds; i++) {
		int listen_fd;

		if (IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3) &&
		    client->is_h3) {
			listen_fd = *client->service->fd_h3;
		} else {
			listen_fd = *client->service->fd;
		}

		if (server_ctx.fds[i].fd == listen_fd) {
			server_ctx.fds[i].events = ZSOCK_POLLIN;
			break;
		}
	}

	for (i = server_ctx.listen_fds; i < server_ctx.client_fds; i++) {
		if (server_ctx.fds[i].fd == client->fd) {
			server_ctx.fds[i].fd = INVALID_SOCK;
			break;
		}
	}

	if (IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3)) {
		for (i = server_ctx.client_fds; i < ARRAY_SIZE(server_ctx.fds); i++) {
			for (int j = 0; j < ARRAY_SIZE(client->h3.stream_sock); j++) {
				if (server_ctx.fds[i].fd == client->h3.stream_sock[j]) {
					server_ctx.fds[i].fd = INVALID_SOCK;

					/* Close the fd to trigger quic_stream_unref() */
					if (client->h3.stream_sock[j] != INVALID_SOCK) {
						(void)zsock_close(client->h3.stream_sock[j]);
						client->h3.stream_sock[j] = INVALID_SOCK;
					}

					break;
				}
			}
		}
	}

	memset(client, 0, sizeof(struct http_client_ctx));
	client->fd = INVALID_SOCK;
}

static void close_client_connection(struct http_client_ctx *client)
{
	int fd = client->fd;

	if (fd >= 0) {
		http_server_release_client(client);

		(void)zsock_close(fd);
	}
}

/*
 * Close an HTTP/3 client, cleaning up all stream fds first so that
 * quic_stream_unref() fires for every stream before the connection fd
 * is closed. For HTTP/1.x and HTTP/2 clients this is identical to
 * close_client_connection().
 */
static void close_h3_or_plain_client(struct http_client_ctx *client,
				     struct zsock_pollfd fds[],
				     int max_fds)
{
	if (IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3) && client->is_h3) {
		h3_client_cleanup(client, fds, max_fds);
	}

	close_client_connection(client);
}

static void client_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct http_client_ctx *client =
		CONTAINER_OF(dwork, struct http_client_ctx, inactivity_timer);

	LOG_DBG("Client %p timeout", client);

	if (IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3) && client->is_h3) {
		LOG_DBG("[%p] %s client connection closing", client, "HTTP/3");

		close_h3_or_plain_client(client, server_ctx.fds, ARRAY_SIZE(server_ctx.fds));
	} else {
		LOG_DBG("[%p] %s client connection closing", client, "HTTP/1.x/2");

		/* Shutdown the socket. This will be detected by poll() and a proper
		 * cleanup will proceed.
		 */
		(void)zsock_shutdown(client->fd, ZSOCK_SHUT_RD);
	}
}

void http_client_timer_restart(struct http_client_ctx *client)
{
	__ASSERT_NO_MSG(IS_ARRAY_ELEMENT(server_ctx.clients, client));

	k_work_reschedule(&client->inactivity_timer, INACTIVITY_TIMEOUT);
}

static const struct http_service_desc *lookup_service(int server_fd)
{
	HTTP_SERVICE_FOREACH(svc) {
		if ((IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_1) ||
		     IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2)) &&
		    *svc->fd == server_fd) {
			return svc;
		}

		if (IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3) &&
		    *svc->fd_h3 == server_fd) {
			return svc;
		}
	}

	return NULL;
}

static bool is_h3_socket(int fd)
{
	if (!IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3)) {
		return false;
	}

	HTTP_SERVICE_FOREACH(svc) {
		if (*svc->fd_h3 == fd) {
			return true;
		}
	}

	return false;
}

static void init_client_ctx(struct http_client_ctx *client, const struct http_service_desc *svc,
			    int new_socket)
{
	client->fd = new_socket;
	client->service = svc;
	client->data_len = 0;
	client->server_state = HTTP_SERVER_PREFACE_STATE;
	client->has_upgrade_header = false;
	client->preface_sent = false;
	client->window_size = HTTP_SERVER_INITIAL_WINDOW_SIZE;

	if (IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3)) {
		client->is_h3 = false;
		client->h3.conn_sock = INVALID_SOCK;

		for (int i = 0; i < ARRAY_SIZE(client->h3.stream_sock); i++) {
			client->h3.stream_sock[i] = INVALID_SOCK;
		}
	}

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

	if (IS_ENABLED(CONFIG_HTTP_SERVER_CAPTURE_HEADERS)) {
		client->header_capture_ctx.count = 0;
		client->header_capture_ctx.cursor = 0;
		client->header_capture_ctx.status = HTTP_HEADER_STATUS_OK;
	}

	if (IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_1) &&
	    strncmp(client->cursor, HTTP2_PREFACE, sizeof(HTTP2_PREFACE) - 1) != 0) {
		return enter_http1_request(client);
	}

	return IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2) ?
		enter_http2_request(client) : -ENOTSUP;
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
			ret = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2) ?
				handle_http_frame_data(client) : -ENOTSUP;
			break;
		case HTTP_SERVER_PREFACE_STATE:
			ret = handle_http_preface(client);
			break;
		case HTTP_SERVER_REQUEST_STATE:
			ret = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_1) ?
				handle_http1_request(client) : -ENOTSUP;
			break;
		case HTTP_SERVER_FRAME_HEADER_STATE:
			ret = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2) ?
				handle_http_frame_header(client) : -ENOTSUP;
			break;
		case HTTP_SERVER_FRAME_HEADERS_STATE:
			ret = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2) ?
				handle_http_frame_headers(client) : -ENOTSUP;
			break;
		case HTTP_SERVER_FRAME_CONTINUATION_STATE:
			ret = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2) ?
				handle_http_frame_continuation(client) : -ENOTSUP;
			break;
		case HTTP_SERVER_FRAME_SETTINGS_STATE:
			ret = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2) ?
				handle_http_frame_settings(client) : -ENOTSUP;
			break;
		case HTTP_SERVER_FRAME_WINDOW_UPDATE_STATE:
			ret = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2) ?
				handle_http_frame_window_update(client) : -ENOTSUP;
			break;
		case HTTP_SERVER_FRAME_RST_STREAM_STATE:
			ret = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2) ?
				handle_http_frame_rst_stream(client) : -ENOTSUP;
			break;
		case HTTP_SERVER_FRAME_GOAWAY_STATE:
			ret = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2) ?
				handle_http_frame_goaway(client) : -ENOTSUP;
			break;
		case HTTP_SERVER_FRAME_PRIORITY_STATE:
			ret = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2) ?
				handle_http_frame_priority(client) : -ENOTSUP;
			break;
		case HTTP_SERVER_FRAME_PADDING_STATE:
			ret = IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2) ?
				handle_http_frame_padding(client) : -ENOTSUP;
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

struct http_client_ctx *h3_find_client(int conn_sock, int *idx)
{
	struct http_server_ctx *ctx = &server_ctx;

	for (int j = ctx->listen_fds; j < ctx->client_fds; j++) {
		struct http_client_ctx *h3_client;

		if (ctx->fds[j].fd == INVALID_SOCK) {
			continue;
		}

		*idx = j - ctx->listen_fds;
		h3_client = &ctx->clients[*idx];

		/* Match by connection socket (all streams on same connection share context) */
		if (h3_client->is_h3 && h3_client->h3.conn_sock == conn_sock) {
			return h3_client;
		}
	}

	return NULL;
}

struct http_client_ctx *get_h3_client_by_stream_fd(struct http_server_ctx *ctx,
						   int stream_fd,
						   int *idx)
{
	for (int i = ctx->listen_fds; i < ctx->client_fds; i++) {
		struct http_client_ctx *h3_client;

		h3_client = &ctx->clients[i - ctx->listen_fds];

		for (int j = 0; j < ARRAY_SIZE(h3_client->h3.stream_sock); j++) {
			if (stream_fd == h3_client->h3.stream_sock[j]) {
				*idx = i - ctx->listen_fds;

				LOG_DBG("Found stream fd %d client #%d",
					stream_fd, *idx);
				return h3_client;
			}
		}
	}

	return NULL;
}

static int add_h3_stream_poll(struct http_client_ctx *client,
			      struct zsock_pollfd *fds,
			      int fd_count,
			      int stream_sock)
{
	int pos = INVALID_SOCK;

	for (int i = 0; i < ARRAY_SIZE(client->h3.stream_sock); i++) {
		if (client->h3.stream_sock[i] == INVALID_SOCK) {
			client->h3.stream_sock[i] = stream_sock;
			pos = i;
			break;
		}
	}

	if (pos == INVALID_SOCK) {
		LOG_ERR("No space for new H3 stream sock in client context");
		return -ENOMEM;
	}

	for (int i = 0; i < fd_count; i++) {
		if (fds[i].fd != INVALID_SOCK) {
			continue;
		}

		fds[i].fd = stream_sock;
		fds[i].events = ZSOCK_POLLIN;
		fds[i].revents = 0;

		return 0;
	}

	client->h3.stream_sock[pos] = INVALID_SOCK;

	return -ENOMEM;
}

static int http_server_run(struct http_server_ctx *ctx)
{
	struct http_client_ctx *client;
	const struct http_service_desc *service;
	zvfs_eventfd_t value;
	bool found_slot;
	int new_socket;
	int ret, i, j;
	int sock_error;
	int idx;
	net_socklen_t optlen = sizeof(int);

	value = 0;

	while (1) {
		ret = zsock_poll(ctx->fds, ARRAY_SIZE(ctx->fds), -1);
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
			zvfs_eventfd_read(ctx->fds[0].fd, &value);
			LOG_DBG("Received stop event. exiting ..");
			ret = 0;
			goto closing;
		}

		/* Only check client sockets in this loop. The HTTP/3 stream
		 * sockets will be handled separately in the loop below this
		 * one.
		 */
		for (i = 1; i < ctx->client_fds; i++) {
			if (ctx->fds[i].fd < 0) {
				continue;
			}

			if (ctx->fds[i].revents & ZSOCK_POLLHUP) {

				if (i >= ctx->listen_fds) {
					LOG_DBG("Client #%d has disconnected",
						i - ctx->listen_fds);
					client = &ctx->clients[i - ctx->listen_fds];

					close_h3_or_plain_client(client, server_ctx.fds,
								 ARRAY_SIZE(server_ctx.fds));
				}

				continue;
			}

			if (ctx->fds[i].revents & ZSOCK_POLLERR) {
				(void)zsock_getsockopt(ctx->fds[i].fd, ZSOCK_SOL_SOCKET,
						       ZSOCK_SO_ERROR, &sock_error, &optlen);
				LOG_DBG("Error on fd %d %d", ctx->fds[i].fd, sock_error);

				if (i >= ctx->listen_fds) {
					client = &ctx->clients[i - ctx->listen_fds];

					close_h3_or_plain_client(client, server_ctx.fds,
								 ARRAY_SIZE(server_ctx.fds));
					continue;
				}

				ret = -sock_error;

				if (ret == -ENETDOWN) {
					LOG_INF("Network is down");
				} else {
					LOG_ERR("Listening socket error, aborting. (%d)", ret);
				}

				goto closing;

			}

			if (!(ctx->fds[i].revents & ZSOCK_POLLIN)) {
				continue;
			}

			/* First check if we have something to accept */
			if (i < ctx->listen_fds) {
				bool is_h3_conn;

				is_h3_conn = COND_CODE_1(CONFIG_HTTP_SERVER_VERSION_3,
							 (is_h3_socket(ctx->fds[i].fd)),
							 (false));

				service = lookup_service(ctx->fds[i].fd);
				if (service == NULL) {
					LOG_ERR("Received event on fd %d which is "
						"not associated with any service",
						ctx->fds[i].fd);
					continue;
				}

				if (service->data->num_clients >= service->concurrent) {
					ctx->fds[i].events = 0;
					continue;
				}

				if (is_h3_conn) {
					/* For HTTP/3, we have two connection sockets,
					 * one listening new connections and one for
					 * accepting new streams. We need to handle them
					 * differently in the code. Only the listening socket
					 * can be checked here because only it is added to the
					 * poll list below listen_fds count.
					 */
					if (ctx->fds[i].fd == *service->fd_h3) {
						/* We are accepting a new QUIC connection */
						LOG_DBG("Accepting new QUIC client %s on fd %d",
							"connection", ctx->fds[i].fd);

						new_socket = accept_h3_connection(ctx->fds[i].fd);
						if (new_socket < 0) {
							LOG_DBG("%saccept fail: %d",
								"H3 conn ", new_socket);
							continue;
						}

						/* A new client is created further down */
						LOG_DBG("New QUIC connection socket %d accepted",
							new_socket);
					} else {
						new_socket = -1;
						errno = ENOENT;
					}
				} else {
					new_socket = accept_new_client(ctx->fds[i].fd);
				}

				if (new_socket < 0) {
					ret = -errno;
					LOG_DBG("%saccept: %d", "", ret);
					continue;
				}

				found_slot = false;

				/* Go through the polled sockets and find a free slot for
				 * the new client.
				 */
				for (j = ctx->listen_fds; j < ctx->client_fds; j++) {
					if (ctx->fds[j].fd != INVALID_SOCK) {
						continue;
					}

					ctx->fds[j].fd = new_socket;
					ctx->fds[j].events = ZSOCK_POLLIN;
					ctx->fds[j].revents = 0;

					service->data->num_clients++;
					idx = j - ctx->listen_fds;

					LOG_DBG("Init client #%d", idx);

					init_client_ctx(&ctx->clients[idx], service, new_socket);

					if (is_h3_conn) {
						ctx->clients[idx].is_h3 = true;
						ctx->clients[idx].server_state =
							HTTP_SERVER_H3_STREAM_STATE;
						ctx->clients[idx].h3.conn_sock = new_socket;

						/* Open our H3 unidirectional streams (control,
						 * QPACK encoder/decoder) for this connection.
						 * We use the connection socket to open
						 * streams on the same QUIC connection.
						 */
						ret = h3_open_uni_streams(&ctx->clients[idx],
									  new_socket);
						if (ret < 0) {
							LOG_DBG("H3: Failed to open uni streams "
								"(%d)", ret);

							/* This is non-fatal, continue without
							 * uni streams
							 */
						}
					}

					found_slot = true;
					break;
				}

				if (!found_slot) {
					LOG_DBG("No free slot found.");
					zsock_close(new_socket);
				}

				continue;
			}

			idx = i - ctx->listen_fds;
			client = &ctx->clients[idx];

			if (IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3) &&
			    quic_is_connection_socket(ctx->fds[i].fd)) {
				/* For HTTP/3, we need to check if the event is for accepting
				 * a new stream on an existing connection. If so, we
				 * need to call accept_h3_stream() to accept the new
				 * stream and add it to the poll list, instead of
				 * handling it as client data.
				 */
				if (ctx->fds[i].revents & ZSOCK_POLLIN) {
					/* We are accepting a stream */
					LOG_DBG("Accepting new QUIC client %s on fd %d",
						"stream", ctx->fds[i].fd);

					new_socket = INVALID_SOCK;

					ret = accept_h3_stream(ctx->fds[i].fd, &new_socket);
					if (ret < 0 && ret != -EAGAIN) {
						/* Connection is closing, clean up the H3 client */
						client = h3_find_client(ctx->fds[i].fd, &idx);
						if (client != NULL) {
							close_h3_or_plain_client(client,
								       server_ctx.fds,
								       ARRAY_SIZE(server_ctx.fds));
						}
					} else if (ret == -EAGAIN) {
						/*
						 * No stream in the accept queue, but POLLIN fired.
						 * This can mean the connection is closed,
						 * quic_endpoint_notify_streams_closed()
						 * gives the semaphore with no stream to wake any
						 * blocked accept, which surfaces here as -EAGAIN.
						 * Check SO_ERROR to tell the two cases apart.
						 */
						optlen = sizeof(sock_error);
						sock_error = 0;
						zsock_getsockopt(ctx->fds[i].fd, ZSOCK_SOL_SOCKET,
								 ZSOCK_SO_ERROR, &sock_error,
								 &optlen);
						if (sock_error != 0) {
							LOG_DBG("QUIC connection fd %d closed "
								"during accept (err=%d)",
								ctx->fds[i].fd, sock_error);

							client = h3_find_client(ctx->fds[i].fd,
										&idx);
							if (client != NULL) {
								close_h3_or_plain_client(client,
								       server_ctx.fds,
								       ARRAY_SIZE(server_ctx.fds));
							} else {
								zsock_close(ctx->fds[i].fd);
								ctx->fds[i].fd = INVALID_SOCK;
								ctx->fds[i].events = 0;
							}
						}
					} else if (new_socket != INVALID_SOCK) {
						/* real stream accepted (bidi or uni) */
						ret = add_h3_stream_poll(client,
							    &ctx->fds[ctx->client_fds],
							    ARRAY_SIZE(ctx->fds) - ctx->client_fds,
							    new_socket);
						if (ret == -ENOMEM) {
							LOG_DBG("No free slot for new H3 stream %d",
								new_socket);
							zsock_close(new_socket);
						}
					} else if (new_socket == INVALID_SOCK) {
						optlen = sizeof(sock_error);
						sock_error = 0;

						zsock_getsockopt(ctx->fds[i].fd, ZSOCK_SOL_SOCKET,
								 ZSOCK_SO_ERROR, &sock_error,
								 &optlen);
						if (sock_error != 0) {
							LOG_DBG("QUIC connection fd %d closed "
								"(err=%d)",
								ctx->fds[i].fd, sock_error);
							client = h3_find_client(ctx->fds[i].fd,
										&idx);
							if (client != NULL) {
								close_h3_or_plain_client(client,
								       server_ctx.fds,
								       ARRAY_SIZE(server_ctx.fds));
							} else {
								/* No client yet, close the
								 * connection fd
								 */
								zsock_close(ctx->fds[i].fd);
								ctx->fds[i].fd = INVALID_SOCK;
								ctx->fds[i].events = 0;
							}
						}
					}
				}

				continue;
			}

			/* This is client data on an established connection (HTTP/1.x or HTTP/2) */

			ret = zsock_recv(client->fd, client->buffer + client->data_len,
					 sizeof(client->buffer) - client->data_len, 0);
			if (ret <= 0) {
				if (ret == 0) {
					LOG_DBG("Connection closed by peer for client #%d",
						i - ctx->listen_fds);
				} else {
					ret = -errno;
					LOG_DBG("[%p] Error reading from socket %d (%d)",
						client, client->fd, ret);
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

				close_h3_or_plain_client(client, server_ctx.fds,
							 ARRAY_SIZE(server_ctx.fds));

			} else if (client->data_len == sizeof(client->buffer)) {
				/* If the RX buffer is still full after parsing,
				 * it means we won't be able to handle this request
				 * with the current buffer size.
				 */
				LOG_ERR("RX buffer too small to handle request");

				close_h3_or_plain_client(client, server_ctx.fds,
							 ARRAY_SIZE(server_ctx.fds));
			}
		}

		if (!IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3)) {
			continue;
		}

		/* Check if any of the HTTP/3 stream sockets have events. */
		for (i = ctx->client_fds; i < ARRAY_SIZE(ctx->fds); i++) {
			int conn_fd;

			if (ctx->fds[i].fd < 0) {
				continue;
			}

			/* TODO: Make sure we get pollhup if stream is closed by peer */
			if (ctx->fds[i].revents & ZSOCK_POLLHUP) {
				LOG_DBG("Stream #%d is closed", i - ctx->client_fds);
				ctx->fds[i].fd = INVALID_SOCK;
				ctx->fds[i].events = 0;
				continue;
			}

			if (!(ctx->fds[i].revents & ZSOCK_POLLIN)) {
				continue;
			}

			client = get_h3_client_by_stream_fd(ctx, ctx->fds[i].fd, &idx);
			if (client == NULL) {
				/* May be a uni stream, try the conn ctx lookup */
				client = h3_find_client_for_uni_stream(ctx->fds[i].fd);
				if (client == NULL) {
					LOG_DBG("No client found for stream fd %d",
						ctx->fds[i].fd);

					/* Nobody owns this fd, remove it to stop the spin */
					ctx->fds[i].fd = INVALID_SOCK;
					ctx->fds[i].events = 0;
					continue;
				}
			}

			if (h3_is_unidirectional_stream(ctx->fds[i].fd)) {
				struct h3_conn_ctx *h3_ctx = h3_get_conn_ctx(client);
				int stream_fd = ctx->fds[i].fd;
				bool identified = h3_ctx != NULL &&
					(stream_fd == h3_ctx->peer_control_stream ||
					 stream_fd == h3_ctx->peer_qpack_encoder_stream ||
					 stream_fd == h3_ctx->peer_qpack_decoder_stream);

				/* Remove from h3.stream_sock[] now so that it is not left in
				 * there if the stream is closed while we are processing it.
				 * It will be added back if it's still open after processing.
				 */
				for (j = 0; j < ARRAY_SIZE(client->h3.stream_sock); j++) {
					if (client->h3.stream_sock[j] == stream_fd) {
						client->h3.stream_sock[j] = INVALID_SOCK;
						break;
					}
				}

				if (!identified) {
					ret = h3_identify_uni_stream(client, stream_fd);
					if (ret == -EAGAIN) {
						/* Put it back, still not identified */
						for (j = 0;
						     j < ARRAY_SIZE(client->h3.stream_sock); j++) {
							if (client->h3.stream_sock[j] ==
							    INVALID_SOCK) {
							      client->h3.stream_sock[j] = stream_fd;
							      break;
							}
						}

						continue;
					}

					if (ret < 0) {
						zsock_close(stream_fd);
						ctx->fds[i].fd = INVALID_SOCK;
						ctx->fds[i].events = 0;
					}

					continue;
				}

				/* Uni stream has its own handler and its own recv buffer */
				ret = h3_handle_uni_stream_data(client, stream_fd);
				if (ret < 0) {
					/* Stream ended or errored, close and remove poll slot */
					zsock_close(stream_fd);
					ctx->fds[i].fd = INVALID_SOCK;
					ctx->fds[i].events = 0;
				} else {
					/* ret == 0 means recv returned EAGAIN (no stream data).
					 * Disable POLLIN to avoid spinning - the fd will be cleaned
					 * up when the connection closes via h3_client_cleanup.
					 */
					ctx->fds[i].events = 0;
				}

				continue;
			}

			/* Bidi request stream: recv into client->buffer and process */
			ret = zsock_recv(ctx->fds[i].fd, client->buffer + client->data_len,
					 sizeof(client->buffer) - client->data_len, 0);
			if (ret <= 0) {
				int closed_fd = ctx->fds[i].fd;

				if (ret == 0) {
					LOG_DBG("Stream closed for client #%d", idx);
				} else {
					ret = -errno;
					LOG_DBG("[%p] Error reading from socket %d (%d)",
						client, ctx->fds[i].fd, ret);
				}

				zsock_close(ctx->fds[i].fd);

				/* Clear the poll slot so the fd number can be safely reused */
				ctx->fds[i].fd = INVALID_SOCK;
				ctx->fds[i].events = 0;
				ctx->fds[i].revents = 0;

				/* Remove from h3.stream_sock[] so a reused fd number
				 * is not mistakenly matched to this client
				 */
				for (int k = 0; k < ARRAY_SIZE(client->h3.stream_sock); k++) {
					if (client->h3.stream_sock[k] == closed_fd) {
						client->h3.stream_sock[k] = INVALID_SOCK;
						break;
					}
				}

				continue;
			}

			client->data_len += ret;

			http_client_timer_restart(client);

			/* Temporarily point client->fd at the stream for sending */
			conn_fd = client->fd;
			client->fd = ctx->fds[i].fd;
			ret = handle_http3_request(client);
			client->fd = conn_fd;

			if (ret < 0 && ret != -EAGAIN) {
				if (ret == -ENOTCONN) {
					LOG_DBG("Client closed connection while handling request");
				} else {
					LOG_ERR("HTTP request handling error (%d)", ret);
				}

				close_h3_or_plain_client(client, server_ctx.fds,
							 ARRAY_SIZE(server_ctx.fds));
			} else if (ret == 0) {
				/*
				 * Response is complete. h3_end_response() already closed the
				 * stream fd via zsock_close(). Just clear the poll slot so we
				 * do not spin on a dead (and potentially reused) fd number.
				 */
				int closed_fd = ctx->fds[i].fd;

				zsock_close(closed_fd); /* safe: shutdown already sent FIN */

				ctx->fds[i].fd = INVALID_SOCK;
				ctx->fds[i].events = 0;
				ctx->fds[i].revents = 0;

				/* Remove from h3.stream_sock[] */
				for (int k = 0; k < ARRAY_SIZE(client->h3.stream_sock); k++) {
					if (client->h3.stream_sock[k] == closed_fd) {
						client->h3.stream_sock[k] = INVALID_SOCK;
						break;
					}
				}

				LOG_DBG("[%p] H3: stream fd %d closed after complete response",
					client, closed_fd);

			} else if (client->data_len == sizeof(client->buffer)) {
				/* If the RX buffer is still full after parsing,
				 * it means we won't be able to handle this request
				 * with the current buffer size.
				 */
				LOG_ERR("RX buffer too small to handle request");

				close_h3_or_plain_client(client, server_ctx.fds,
							 ARRAY_SIZE(server_ctx.fds));
			}
		}
	}

	return 0;

closing:
	/* Close all client connections and the server socket */
	close_all_sockets(ctx);
	return ret;
}

/* Compare a path and a resource string. The path string comes from the HTTP request and may be
 * terminated by either '?' or '\0'. The resource string is registered along with the resource and
 * may only be terminated by `\0`.
 */
static int compare_strings(const char *path, const char *resource)
{
	while ((*path && *resource) && (*path == *resource) && (*path != '?')) {
		path++;
		resource++;
	}

	/* Check if both strings have reached their terminators */
	if ((*path == '\0' || *path == '?') && (*resource == '\0')) {
		return 0; /* Strings are equal */
	}

	return 1; /* Strings are not equal */
}

static int path_len_without_query(const char *path)
{
	int len = 0;

	while ((path[len] != '\0') && (path[len] != '?')) {
		len++;
	}

	return len;
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

/* Resolves '.' / '..' in the path portion of the URL (everything before the first '?' or '#')
 * so the caller-visible buffer holds a canonical form. The query/fragment portion is preserved
 * unchanged. The output is always shorter or equal in length, so the in-place rewrite is safe.
 */
void http_server_remove_dot_segments(char *path)
{
	char *in = path;
	char *out = path;
	char *query = strpbrk(path, "?#");
	size_t qtail_len = (query != NULL) ? strlen(query) : 0;

	while ((*in != '\0') && (in != query)) {
		if (in[0] == '.') {
			if (in[1] == '/') {
				in += 2;
				continue;
			} else if ((in[1] == '.') && (in[2] == '/')) {
				in += 3;
				continue;
			} else if ((in[1] == '\0') || (in + 1 == query)) {
				in += 1;
				continue;
			} else if ((in[1] == '.') &&
					((in[2] == '\0') || (in + 2 == query))) {
				in += 2;
				continue;
			}
		} else if ((in[0] == '/') && (in[1] == '.')) {
			if (in[2] == '/') {
				in += 2;
				continue;
			} else if ((in[2] == '\0') || ((in + 2) == query)) {
				in += 2;
				*out++ = '/';
				continue;
			} else if ((in[2] == '.') && (in[3] == '/')) {
				in += 3;
				while ((out > path) && (*(out - 1) != '/')) {
					out--;
				}
				if (out > path) {
					out--;
				}
				continue;
			} else if ((in[2] == '.') && ((in[3] == '\0') || ((in + 3) == query))) {
				in += 3;
				while ((out > path) && (*(out - 1) != '/')) {
					out--;
				}
				if (out > path) {
					out--;
				}
				*out++ = '/';
				continue;
			}
		}

		/* Move the first segment to output: leading char (often '/') plus
		 * all non-'/' bytes up to the next segment boundary.
		 */
		*out++ = *in++;
		while ((*in != '\0') && (in != query) && (*in != '/')) {
			*out++ = *in++;
		}
	}

	if (qtail_len > 0) {
		memmove(out, query, qtail_len);
		out += qtail_len;
	}
	*out = '\0';
}

struct http_resource_detail *get_resource_detail(const struct http_service_desc *service,
						 const char *path, int *path_len, bool is_websocket)
{
	HTTP_SERVICE_FOREACH_RESOURCE(service, resource) {
		if (skip_this(resource, is_websocket)) {
			continue;
		}

		if (compare_strings(path, resource->resource) == 0) {
			NET_DBG("Got match for %s", resource->resource);

			*path_len = strlen(resource->resource);
			return resource->detail;
		}
	}

	if (IS_ENABLED(CONFIG_HTTP_SERVER_RESOURCE_WILDCARD)) {
		HTTP_SERVICE_FOREACH_RESOURCE(service, resource) {
			int ret;

			if (skip_this(resource, is_websocket)) {
				continue;
			}

			ret = fnmatch(resource->resource, path, (FNM_PATHNAME | FNM_LEADING_DIR));
			if (ret == 0) {
				*path_len = path_len_without_query(path);
				return resource->detail;
			}
		}
	}

	if (service->res_fallback != NULL) {
		*path_len = path_len_without_query(path);
		return service->res_fallback;
	}

	NET_DBG("No match for %s", path);

	return NULL;
}

int http_server_find_file(char *fname, size_t fname_size, size_t *file_size,
			  uint8_t supported_compression, enum http_compression *chosen_compression)
{
	struct fs_dirent dirent;
	int ret;

	if (IS_ENABLED(CONFIG_HTTP_SERVER_COMPRESSION)) {
		const size_t len = strlen(fname);
		*chosen_compression = HTTP_NONE;
		if (IS_BIT_SET(supported_compression, HTTP_BR)) {
			snprintk(fname + len, fname_size - len, ".br");
			ret = fs_stat(fname, &dirent);
			if (ret == 0) {
				*chosen_compression = HTTP_BR;
				goto return_filename;
			}
		}
		if (IS_BIT_SET(supported_compression, HTTP_GZIP)) {
			snprintk(fname + len, fname_size - len, ".gz");
			ret = fs_stat(fname, &dirent);
			if (ret == 0) {
				*chosen_compression = HTTP_GZIP;
				goto return_filename;
			}
		}
		if (IS_BIT_SET(supported_compression, HTTP_ZSTD)) {
			snprintk(fname + len, fname_size - len, ".zst");
			ret = fs_stat(fname, &dirent);
			if (ret == 0) {
				*chosen_compression = HTTP_ZSTD;
				goto return_filename;
			}
		}
		if (IS_BIT_SET(supported_compression, HTTP_COMPRESS)) {
			snprintk(fname + len, fname_size - len, ".lzw");
			ret = fs_stat(fname, &dirent);
			if (ret == 0) {
				*chosen_compression = HTTP_COMPRESS;
				goto return_filename;
			}
		}
		if (IS_BIT_SET(supported_compression, HTTP_DEFLATE)) {
			snprintk(fname + len, fname_size - len, ".zz");
			ret = fs_stat(fname, &dirent);
			if (ret == 0) {
				*chosen_compression = HTTP_DEFLATE;
				goto return_filename;
			}
		}
		/* No compressed file found, try the original filename */
		fname[len] = '\0';
	}
	ret = fs_stat(fname, &dirent);
	if (ret != 0) {
		return -ENOENT;
	}

return_filename:
	*file_size = dirent.size;
	return ret;
}

void http_server_get_content_type_from_extension(char *url, char *content_type,
						 size_t content_type_size)
{
	size_t url_len = strlen(url);

	HTTP_SERVER_CONTENT_TYPE_FOREACH(ct) {
		char *ext;

		if (url_len <= ct->extension_len) {
			continue;
		}

		ext = &url[url_len - ct->extension_len];

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

bool http_response_is_final(struct http_response_ctx *rsp, enum http_transaction_status status)
{
	if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
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

void populate_request_ctx(struct http_request_ctx *req_ctx, uint8_t *data, size_t len,
			  struct http_header_capture_ctx *header_ctx)
{
	req_ctx->data = data;
	req_ctx->data_len = len;

	if (NULL == header_ctx || header_ctx->status == HTTP_HEADER_STATUS_NONE) {
		req_ctx->headers = NULL;
		req_ctx->header_count = 0;
		req_ctx->headers_status = HTTP_HEADER_STATUS_NONE;
	} else {
		req_ctx->headers = header_ctx->headers;
		req_ctx->header_count = header_ctx->count;
		req_ctx->headers_status = header_ctx->status;
	}
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
	zvfs_eventfd_write(server_ctx.fds[0].fd, 1);

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
				LOG_ERR("Failed to initialize HTTP server");
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
