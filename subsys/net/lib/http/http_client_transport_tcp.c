/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/client_multiprotocol.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>


LOG_MODULE_DECLARE(net_http_client_mp, CONFIG_NET_HTTP_LOG_LEVEL);

#define INVALID_SOCK (-1)
#define TCP_MAX_INSTANCES CONFIG_HTTP_CLIENT_MAX_INSTANCES
#define TCP_SEND_TIMEOUT K_MSEC(CONFIG_HTTP_CLIENT_SEND_TIMEOUT)

/* Bookkeeping for asynchronous reception. The socket service callback shares a
 * single user_data per registration, so the triggered socket is matched against
 * this registry to find the owning client instance.
 */
static struct http_client *tcp_clients[TCP_MAX_INSTANCES];
static struct zsock_pollfd tcp_fds[TCP_MAX_INSTANCES];
static K_MUTEX_DEFINE(tcp_lock);

/* The client a notification is in progress for and the thread delivering it.
 * The socket service delivers one notification at a time.
 */
static struct http_client *tcp_notifying;
static k_tid_t tcp_notifying_thread;
static K_CONDVAR_DEFINE(tcp_notified);

static void tcp_transport_recv_cb(struct net_socket_service_event *pev);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(tcp_socket_service, tcp_transport_recv_cb,
				      TCP_MAX_INSTANCES);

/* Both transports keep their socket in a TCP context; the TLS context embeds
 * one. The secure flag tells the two apart.
 */
static struct http_client_transport_tcp_ctx *sock_ctx(struct http_client *client)
{
	if (IS_ENABLED(CONFIG_HTTP_CLIENT_TRANSPORT_TLS) && client->transport_ops->secure) {
		struct http_client_transport_tls_ctx *tls_ctx = client->transport_ctx;

		return &tls_ctx->tcp;
	}

	return client->transport_ctx;
}

static int tcp_client_sock(struct http_client *client)
{
	return sock_ctx(client)->sock;
}

/* Rebuild the pollable socket array from the registry and (re)register it with
 * the socket service. Must be called with tcp_lock held.
 */
static int tcp_sync_locked(void)
{
	bool any = false;

	for (int i = 0; i < TCP_MAX_INSTANCES; i++) {
		if (tcp_clients[i] != NULL) {
			tcp_fds[i].fd = tcp_client_sock(tcp_clients[i]);
			tcp_fds[i].events = ZSOCK_POLLIN;
			any = true;
		} else {
			tcp_fds[i].fd = INVALID_SOCK;
			tcp_fds[i].events = 0;
		}
	}

	if (!any) {
		return net_socket_service_unregister(&tcp_socket_service);
	}

	return net_socket_service_register(&tcp_socket_service, tcp_fds,
					   ARRAY_SIZE(tcp_fds), NULL);
}

static int tcp_register(struct http_client *client)
{
	int ret = -ENOMEM;
	int slot;

	k_mutex_lock(&tcp_lock, K_FOREVER);

	for (slot = 0; slot < TCP_MAX_INSTANCES; slot++) {
		if (tcp_clients[slot] == NULL) {
			break;
		}
	}

	if (slot == TCP_MAX_INSTANCES) {
		NET_ERR("No free slot for asynchronous TCP transport, "
			"increase %s", "CONFIG_HTTP_CLIENT_MAX_INSTANCES");
		goto out;
	}

	tcp_clients[slot] = client;

	ret = tcp_sync_locked();
	if (ret < 0) {
		NET_ERR("Failed to register socket service, err %d", ret);
		tcp_clients[slot] = NULL;
	}

out:
	k_mutex_unlock(&tcp_lock);

	return ret;
}

static void tcp_unregister(struct http_client *client)
{
	bool found = false;

	k_mutex_lock(&tcp_lock, K_FOREVER);

	for (int i = 0; i < TCP_MAX_INSTANCES; i++) {
		if (tcp_clients[i] == client) {
			tcp_clients[i] = NULL;
			found = true;
		}
	}

	if (found) {
		(void)tcp_sync_locked();
	}

	k_mutex_unlock(&tcp_lock);
}

/* Map poll result flags to a reception status: 0 for data or a peer close,
 * a negative errno for an error condition.
 */
static int tcp_revents_status(int fd, short revents)
{
	if ((revents & ZSOCK_POLLNVAL) != 0) {
		return -EBADF;
	}

	if ((revents & ZSOCK_POLLERR) != 0) {
		int sock_err = 0;
		net_socklen_t optlen = sizeof(sock_err);
		int ret;

		ret = zsock_getsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_ERROR, &sock_err, &optlen);

		return (ret == 0 && sock_err > 0) ? -sock_err : -EIO;
	}

	return 0;
}

static void tcp_transport_recv_cb(struct net_socket_service_event *pev)
{
	short revents = pev->event.revents;
	struct http_client *client = NULL;
	int status;

	k_mutex_lock(&tcp_lock, K_FOREVER);

	for (int i = 0; i < TCP_MAX_INSTANCES; i++) {
		if (tcp_clients[i] != NULL &&
		    tcp_fds[i].fd == pev->event.fd) {
			client = tcp_clients[i];
			break;
		}
	}

	/* Visible to tcp_wait_notified() until the client is notified. */
	tcp_notifying = client;
	tcp_notifying_thread = k_current_get();

	k_mutex_unlock(&tcp_lock);

	if (client == NULL) {
		NET_DBG("No client found for socket %d", pev->event.fd);
		goto done;
	}

	status = tcp_revents_status(pev->event.fd, revents);
	if (status < 0) {
		NET_ERR("Error condition on socket %d (revents 0x%x), err %d",
			pev->event.fd, revents, status);
	}

	/* Nothing to do if there is no new data and no error condition. */
	if (status != 0 || (revents & (ZSOCK_POLLIN | ZSOCK_POLLHUP)) != 0) {
		http_client_received_data(client, status);
	}

done:
	k_mutex_lock(&tcp_lock, K_FOREVER);
	tcp_notifying = NULL;
	k_condvar_broadcast(&tcp_notified);
	k_mutex_unlock(&tcp_lock);
}

/* Wait until a notification dispatched for the client, if any, has returned.
 * Called with the client lock released, so that the notification can take it
 * and find the session closed. The notifying thread itself, closing the
 * transport of a session it ended, is not waited for.
 */
static void tcp_wait_notified(struct http_client *client)
{
	k_mutex_lock(&tcp_lock, K_FOREVER);

	while (tcp_notifying == client && tcp_notifying_thread != k_current_get()) {
		(void)k_condvar_wait(&tcp_notified, &tcp_lock, K_FOREVER);
	}

	k_mutex_unlock(&tcp_lock);
}

/* Longest domain name a TLS server name may carry, with its terminator. */
#define TLS_SERVER_NAME_MAX 256

#if defined(CONFIG_HTTP_CLIENT_TRANSPORT_TLS)
/* With the socket dispatcher, the first option set on a socket binds it to a
 * backend: native TLS is requested before anything else.
 */
static int tls_select_native(int sock, const struct http_client_transport_tls_config *tls)
{
	int tls_native = 1;
	int ret;

	if (!IS_ENABLED(CONFIG_NET_SOCKETS_OFFLOAD_DISPATCHER) || !tls->set_native_tls) {
		return 0;
	}

	ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_NATIVE, &tls_native,
			       sizeof(tls_native));
	if (ret < 0) {
		ret = -errno;
		NET_ERR("Failed to set %s, err %d", "TLS_NATIVE", ret);
		return ret;
	}

	return 0;
}

/* The name to send as the TLS server name: the configured hostname, else the
 * session host when that is a domain name (RFC 9113, Section 9.2). An IP
 * literal is not sent (RFC 6066, Section 3).
 */
static const char *tls_server_name(const struct http_client *client,
				   const struct http_client_transport_tls_config *tls,
				   char *buf, size_t size)
{
	const char *host = client->session.host;
	struct net_in_addr addr4;
	size_t len;

	if (tls->hostname != NULL) {
		return tls->hostname;
	}

	if (host == NULL || host[0] == '[') {
		return NULL;
	}

	len = strcspn(host, ":");
	if (len == 0 || len >= size) {
		return NULL;
	}

	memcpy(buf, host, len);
	buf[len] = '\0';

	if (net_addr_pton(NET_AF_INET, buf, &addr4) == 0) {
		return NULL;
	}

	return buf;
}

static int tls_setup(struct http_client *client, int sock,
		     const struct http_client_transport_tls_config *tls)
{
	static const char *const alpn_h2[] = { "h2" };
	static const char *const alpn_http1[] = { "http/1.1" };
	const char *const *alpn = (client->version == HTTP_CLIENT_VERSION_2) ? alpn_h2 : alpn_http1;
	char name[TLS_SERVER_NAME_MAX];
	const char *server_name;
	int ret;

	if (tls->cipher_list != NULL && tls->cipher_count > 0) {
		ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_CIPHERSUITE_LIST,
				       tls->cipher_list, sizeof(int) * tls->cipher_count);
		if (ret < 0) {
			ret = -errno;
			NET_ERR("Failed to set %s, err %d", "TLS_CIPHERSUITE_LIST", ret);
			return ret;
		}
	}

	if (tls->sec_tag_list != NULL && tls->sec_tag_count > 0) {
		ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
				       tls->sec_tag_list,
				       sizeof(sec_tag_t) * tls->sec_tag_count);
		if (ret < 0) {
			ret = -errno;
			NET_ERR("Failed to set %s, err %d", "TLS_SEC_TAG_LIST", ret);
			return ret;
		}
	}

	/* HTTP/2 over TLS is negotiated with ALPN (RFC 9113, Section 3.3). */
	if (IS_ENABLED(CONFIG_HTTP_CLIENT_TLS_USE_ALPN)) {
		ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_ALPN_LIST, alpn,
				       sizeof(const char *));
		if (ret < 0) {
			ret = -errno;
			NET_ERR("Failed to set %s, err %d", "TLS_ALPN_LIST", ret);
			return ret;
		}
	}

	server_name = tls_server_name(client, tls, name, sizeof(name));
	if (server_name != NULL) {
		ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME,
				       server_name, strlen(server_name) + 1);
		if (ret < 0) {
			ret = -errno;
			NET_ERR("Failed to set %s, err %d", "TLS_HOSTNAME", ret);
			return ret;
		}
	}

	if (tls->cert_nocopy != ZSOCK_TLS_CERT_NOCOPY_NONE) {
		ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_CERT_NOCOPY,
				       &tls->cert_nocopy, sizeof(tls->cert_nocopy));
		if (ret < 0) {
			ret = -errno;
			NET_ERR("Failed to set %s, err %d", "TLS_CERT_NOCOPY", ret);
			return ret;
		}
	}

	return 0;
}
#else
static int tls_select_native(int sock, const struct http_client_transport_tls_config *tls)
{
	ARG_UNUSED(sock);
	ARG_UNUSED(tls);

	return -ENOTSUP;
}

static int tls_setup(struct http_client *client, int sock,
		     const struct http_client_transport_tls_config *tls)
{
	ARG_UNUSED(client);
	ARG_UNUSED(sock);
	ARG_UNUSED(tls);

	return -ENOTSUP;
}
#endif /* CONFIG_HTTP_CLIENT_TRANSPORT_TLS */

static void tcp_transport_init(void *transport_ctx)
{
	struct http_client_transport_tcp_ctx *ctx = transport_ctx;

	NET_ASSERT(ctx != NULL);

	ctx->sock = INVALID_SOCK;
}

/* Create, configure and connect the socket. A non-NULL tls selects a TLS
 * socket configured accordingly.
 */
static int sock_connect(struct http_client *client,
			struct http_client_transport_tcp_ctx *ctx,
			const struct http_client_transport_tls_config *tls)
{
	int family = client->session.server_addr.ss_family;
	int proto = (tls != NULL) ? NET_IPPROTO_TLS_1_2 : NET_IPPROTO_TCP;
	int ret;

	NET_ASSERT(ctx != NULL);

	if (ctx->socket_create != NULL) {
		ret = ctx->socket_create(client, family, proto);
		if (ret < 0) {
			return ret;
		}

		ctx->sock = ret;
	} else {
		ctx->sock = zsock_socket(family, NET_SOCK_STREAM, proto);
		if (ctx->sock < 0) {
			return -errno;
		}
	}

	NET_DBG("Created socket %d", ctx->sock);

	/* The interface goes before the TLS options: with the socket
	 * dispatcher, any of them would bind the socket to the default
	 * backend first. Only native TLS, a backend choice itself, precedes it.
	 */
	if (tls != NULL) {
		ret = tls_select_native(ctx->sock, tls);
		if (ret < 0) {
			goto error;
		}
	}

	if (ctx->if_name != NULL) {
		struct net_ifreq ifname = { 0 };

		strncpy(ifname.ifr_name, ctx->if_name, sizeof(ifname.ifr_name) - 1);

		ret = zsock_setsockopt(ctx->sock, ZSOCK_SOL_SOCKET,
				       ZSOCK_SO_BINDTODEVICE, &ifname,
				       sizeof(struct net_ifreq));
		if (ret < 0) {
			ret = -errno;
			NET_ERR("Failed to bind to interface %s error (%d)",
				ifname.ifr_name, ret);
			goto error;
		}

		NET_DBG("Bound to interface %s", ifname.ifr_name);
	}

	if (tls != NULL) {
		ret = tls_setup(client, ctx->sock, tls);
		if (ret < 0) {
			goto error;
		}
	}

	ret = zsock_connect(ctx->sock, (const struct net_sockaddr *)&client->session.server_addr,
			    client->session.addrlen);
	if (ret < 0) {
		ret = -errno;
		goto error;
	}

	/* The protocol layers send small control frames, flow control credit
	 * among them, that the peer waits for. Held back by Nagle's algorithm
	 * until the peer's delayed acknowledgment, they would stall the
	 * transfer for a round trip plus the delay, every window.
	 */
	ret = zsock_setsockopt(ctx->sock, NET_IPPROTO_TCP, ZSOCK_TCP_NODELAY, &(int){ 1 },
			       sizeof(int));
	if (ret < 0) {
		NET_DBG("TCP_NODELAY not supported, err %d", -errno);
	}

	/* Synchronous sessions receive on the calling thread. */
	if (client->session.async) {
		ret = tcp_register(client);
		if (ret < 0) {
			NET_ERR("Failed to setup async reception, err %d", ret);
			goto error;
		}
	}

	NET_DBG("Connect completed");

	return 0;

error:
	(void)zsock_close(ctx->sock);
	ctx->sock = INVALID_SOCK;

	return ret;
}

static int tcp_transport_connect(struct http_client *client)
{
	return sock_connect(client, client->transport_ctx, NULL);
}

static int tcp_transport_send(struct http_client *client, void *stream, const uint8_t *data,
			      size_t len, bool end_stream)
{
	int ret;

	ARG_UNUSED(stream);
	ARG_UNUSED(end_stream);

	ret = zsock_send_all(tcp_client_sock(client), data, len, 0, TCP_SEND_TIMEOUT, NULL);
	if (ret < 0) {
		NET_ERR("Failed to send, err %d", ret);
	}

	return ret;
}

static int tcp_transport_recv(struct http_client *client, void **stream, uint8_t *buf,
			      size_t bufsize, bool *end_stream)
{
	int ret;

	*stream = NULL;
	*end_stream = false;

	ret = zsock_recv(tcp_client_sock(client), buf, bufsize, ZSOCK_MSG_DONTWAIT);
	if (ret < 0) {
		ret = -errno;
		if (ret != -EAGAIN) {
			NET_ERR("Failed to receive, err %d", ret);
		}
	}

	return ret;
}

static int tcp_transport_wait(struct http_client *client, k_timeout_t timeout)
{
	struct zsock_pollfd pfd = {
		.fd = tcp_client_sock(client),
		.events = ZSOCK_POLLIN,
	};
	int timeout_ms = K_TIMEOUT_EQ(timeout, K_FOREVER) ? -1 :
			 (int)k_ticks_to_ms_ceil32(timeout.ticks);
	int ret;

	if (pfd.fd < 0) {
		return -ENOTCONN;
	}

	ret = zsock_poll(&pfd, 1, timeout_ms);
	if (ret < 0) {
		return -errno;
	}

	if (ret == 0) {
		return -EAGAIN;
	}

	return tcp_revents_status(pfd.fd, pfd.revents);
}

static void tcp_transport_stop(struct http_client *client)
{
	tcp_unregister(client);
}

static int tcp_transport_close(struct http_client *client)
{
	struct http_client_transport_tcp_ctx *ctx = sock_ctx(client);
	int ret;

	/* Remove the socket from the socket service before closing it, so that
	 * the service no longer polls a closed/recycled file descriptor, and
	 * let a notification already dispatched for it return.
	 */
	tcp_unregister(client);
	tcp_wait_notified(client);

	NET_DBG("Closing socket %d", ctx->sock);

	ret = zsock_close(ctx->sock);
	if (ret < 0) {
		return -errno;
	}

	ctx->sock = INVALID_SOCK;

	return 0;
}

static const struct http_client_transport_ops http_transport_tcp = {
	.secure = false,
	.init = tcp_transport_init,
	.connect = tcp_transport_connect,
	.send = tcp_transport_send,
	.recv = tcp_transport_recv,
	.wait = tcp_transport_wait,
	.stop = tcp_transport_stop,
	.close = tcp_transport_close,
};

#if defined(CONFIG_HTTP_CLIENT_TRANSPORT_TLS)
static void tls_transport_init(void *transport_ctx)
{
	struct http_client_transport_tls_ctx *ctx = transport_ctx;

	NET_ASSERT(ctx != NULL);

	tcp_transport_init(&ctx->tcp);
}

static int tls_transport_connect(struct http_client *client)
{
	struct http_client_transport_tls_ctx *ctx = client->transport_ctx;

	return sock_connect(client, &ctx->tcp, &ctx->config);
}

static const struct http_client_transport_ops http_transport_tls = {
	.secure = true,
	.init = tls_transport_init,
	.connect = tls_transport_connect,
	.send = tcp_transport_send,
	.recv = tcp_transport_recv,
	.wait = tcp_transport_wait,
	.stop = tcp_transport_stop,
	.close = tcp_transport_close,
};
#endif /* CONFIG_HTTP_CLIENT_TRANSPORT_TLS */

/* Public API */

int http_client_init_tcp(struct http_client *client, enum http_client_version version,
			 struct http_client_transport_tcp_ctx *transport_ctx)
{
	if (transport_ctx == NULL) {
		return -EINVAL;
	}

	return http_client_init(client, version, &http_transport_tcp,
				transport_ctx);
}

#if defined(CONFIG_HTTP_CLIENT_TRANSPORT_TLS)
int http_client_init_tls(struct http_client *client, enum http_client_version version,
			 struct http_client_transport_tls_ctx *transport_ctx)
{
	if (transport_ctx == NULL) {
		return -EINVAL;
	}

	return http_client_init(client, version, &http_transport_tls,
				transport_ctx);
}
#endif /* CONFIG_HTTP_CLIENT_TRANSPORT_TLS */
