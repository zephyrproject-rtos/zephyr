/* server.c */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_APP)
#define SYS_LOG_DOMAIN "net/app"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>

#include <net/net_app.h>

#include "net_app_private.h"

#if defined(CONFIG_NET_TCP)
static void new_client(struct net_context *net_ctx,
		       const struct sockaddr *addr)
{
#if defined(CONFIG_NET_DEBUG_APP) && (CONFIG_SYS_LOG_NET_LEVEL > 2)
#if defined(CONFIG_NET_IPV6)
#define PORT_STR sizeof("[]:xxxxx")
	char buf[NET_IPV6_ADDR_LEN + PORT_STR];
#elif defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
#define PORT_STR sizeof(":xxxxx")
	char buf[NET_IPV4_ADDR_LEN + PORT_STR];
#endif

	NET_INFO("Connection from %s (%p)",
		 _net_app_sprint_ipaddr(buf, sizeof(buf), addr),
		 net_ctx);
#endif /* CONFIG_NET_DEBUG_APP */
}

void _net_app_accept_cb(struct net_context *net_ctx,
			struct sockaddr *addr,
			socklen_t addrlen,
			int status, void *data)
{
	struct net_app_ctx *ctx = data;

	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	if (status != 0 || ctx->server.net_ctx) {
		/* We are already connected and support only one connection at
		 * a time so this new connection must be closed.
		 */
		net_context_put(net_ctx);

		if (ctx->cb.connect) {
			if (!status) {
				status = -ECONNREFUSED;
			}

			ctx->cb.connect(ctx, status, ctx->user_data);
		}

		if (ctx->server.net_ctx) {
			NET_DBG("Already connected via context %p",
				ctx->server.net_ctx);
		}

		return;
	}

	ctx->server.net_ctx = net_ctx;

	new_client(net_ctx, addr);

	net_context_recv(net_ctx, ctx->recv_cb, K_NO_WAIT, ctx);

	if (ctx->cb.connect) {
		ctx->cb.connect(ctx, 0, ctx->user_data);
	}
}
#endif /* CONFIG_NET_TCP */

int net_app_listen(struct net_app_ctx *ctx)
{
	int ret;
	bool dual = false;

	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	if (ctx->app_type != NET_APP_SERVER) {
		return -EINVAL;
	}

#if defined(CONFIG_NET_IPV4)
	if (ctx->ipv4.local.family == AF_UNSPEC) {
		ctx->ipv4.local.family = AF_INET;
		dual = true;

		_net_app_set_local_addr(&ctx->ipv4.local, NULL,
					net_sin(&ctx->ipv4.local)->sin_port);
	}

	ret = _net_app_set_net_ctx(ctx, ctx->ipv4.ctx, &ctx->ipv4.local,
				   sizeof(struct sockaddr_in), ctx->proto);
	if (ret < 0) {
		net_context_put(ctx->ipv4.ctx);
		ctx->ipv4.ctx = NULL;
	}
#endif

	/* We ignore the IPv4 error if IPv6 is enabled */

#if defined(CONFIG_NET_IPV6)
	if (ctx->ipv6.local.family == AF_UNSPEC || dual) {
		ctx->ipv6.local.family = AF_INET6;

		_net_app_set_local_addr(&ctx->ipv6.local, NULL,
				       net_sin6(&ctx->ipv6.local)->sin6_port);
	}

	ret = _net_app_set_net_ctx(ctx, ctx->ipv6.ctx, &ctx->ipv6.local,
				   sizeof(struct sockaddr_in6), ctx->proto);
	if (ret < 0) {
		net_context_put(ctx->ipv6.ctx);
		ctx->ipv6.ctx = NULL;
	}
#endif

	return ret;
}

int net_app_init_server(struct net_app_ctx *ctx,
			enum net_sock_type sock_type,
			enum net_ip_protocol proto,
			struct sockaddr *server_addr,
			u16_t port,
			void *user_data)
{
	int ret;

	if (!ctx) {
		return -EINVAL;
	}

	if (ctx->is_init) {
		return -EALREADY;
	}

#if defined(CONFIG_NET_IPV4)
	memset(&ctx->ipv4.local, 0, sizeof(ctx->ipv4.local));
#endif
#if defined(CONFIG_NET_IPV6)
	memset(&ctx->ipv6.local, 0, sizeof(ctx->ipv6.local));
#endif

	if (server_addr) {
		if (server_addr->family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
			memcpy(&ctx->ipv4.local, server_addr,
			       sizeof(ctx->ipv4.local));
#else
			return -EPROTONOSUPPORT;
#endif
		}

		if (server_addr->family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
			memcpy(&ctx->ipv6.local, server_addr,
			       sizeof(ctx->ipv6.local));
#else
			return -EPROTONOSUPPORT;
#endif
		}
	} else {
		if (port == 0) {
			return -EINVAL;
		}

#if defined(CONFIG_NET_IPV4)
		ctx->ipv4.local.family = AF_INET;
		net_sin(&ctx->ipv4.local)->sin_port = htons(port);
#endif
#if defined(CONFIG_NET_IPV6)
		ctx->ipv6.local.family = AF_INET6;
		net_sin6(&ctx->ipv6.local)->sin6_port = htons(port);
#endif
	}

	ctx->app_type = NET_APP_SERVER;
	ctx->user_data = user_data;
	ctx->send_data = net_context_sendto;
	ctx->recv_cb = _net_app_received;
	ctx->proto = proto;
	ctx->sock_type = sock_type;

	ret = _net_app_config_local_ctx(ctx, sock_type, proto, NULL);
	if (ret < 0) {
		goto fail;
	}

	NET_ASSERT_INFO(ctx->default_ctx, "Default ctx not selected");

	ctx->is_init = true;

fail:
	return ret;
}

#if defined(CONFIG_NET_APP_TLS)
static inline void new_server(struct net_app_ctx *ctx,
			      const char *server_banner)
{
#if defined(CONFIG_NET_DEBUG_APP) && (CONFIG_SYS_LOG_NET_LEVEL > 2)
#if defined(CONFIG_NET_IPV6)
#define PORT_STR sizeof("[]:xxxxx")
	char buf[NET_IPV6_ADDR_LEN + PORT_STR];
#elif defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
#define PORT_STR sizeof(":xxxxx")
	char buf[NET_IPV4_ADDR_LEN + PORT_STR];
#endif

#if defined(CONFIG_NET_IPV6)
	NET_INFO("%s %s (%p)", server_banner,
		 _net_app_sprint_ipaddr(buf, sizeof(buf), &ctx->ipv6.local),
		 ctx);
#endif

#if defined(CONFIG_NET_IPV4)
	NET_INFO("%s %s (%p)", server_banner,
		 _net_app_sprint_ipaddr(buf, sizeof(buf), &ctx->ipv4.local),
		 ctx);
#endif
#endif /* CONFIG_NET_DEBUG_APP */
}

static void tls_server_handler(struct net_app_ctx *ctx,
			       struct k_sem *startup_sync)
{
	int ret;

	NET_DBG("Starting TLS server thread for %p", ctx);

	ret = _net_app_tls_init(ctx, MBEDTLS_SSL_IS_SERVER);
	if (ret < 0) {
		NET_DBG("TLS server init failed");
		return;
	}

	k_sem_give(startup_sync);

	while (1) {
		_net_app_ssl_mainloop(ctx);

		mbedtls_ssl_close_notify(&ctx->tls.mbedtls.ssl);

		if (ctx->cb.close) {
			ctx->cb.close(ctx, -ESHUTDOWN, ctx->user_data);
		}

		if (ctx->server.net_ctx) {
			NET_DBG("Server context %p removed",
				ctx->server.net_ctx);
			net_context_put(ctx->server.net_ctx);
			ctx->server.net_ctx = NULL;
		}
	}
}

#define TLS_STARTUP_TIMEOUT K_SECONDS(5)

bool net_app_server_tls_enable(struct net_app_ctx *ctx)
{
	struct k_sem startup_sync;

	NET_ASSERT(ctx);

	if (!(ctx->tls.stack && ctx->tls.stack_size > 0)) {
		/* No stack or stack size is 0, we cannot enable */
		return false;
	}

	ctx->is_enabled = true;

	/* Start the thread that handles TLS traffic. */
	if (!ctx->tls.tid) {
		k_sem_init(&startup_sync, 0, 1);

		ctx->tls.tid = k_thread_create(&ctx->tls.thread,
					       ctx->tls.stack,
					       ctx->tls.stack_size,
					       (k_thread_entry_t)
							tls_server_handler,
					       ctx, &startup_sync, 0,
					       K_PRIO_COOP(7), 0, 0);

		/* Wait until we know that the TLS thread startup was ok */
		if (k_sem_take(&startup_sync, TLS_STARTUP_TIMEOUT) < 0) {
			NET_ERR("TLS server handler start failed");
			_net_app_tls_handler_stop(ctx);
			return false;
		}
	}

	return true;
}

bool net_app_server_tls_disable(struct net_app_ctx *ctx)
{
	NET_ASSERT(ctx);

	ctx->is_enabled = false;

	if (!ctx->tls.tid) {
		return false;
	}

	_net_app_tls_handler_stop(ctx);

	return true;
}

int net_app_server_tls(struct net_app_ctx *ctx,
		       u8_t *request_buf,
		       size_t request_buf_len,
		       const char *server_banner,
		       u8_t *personalization_data,
		       size_t personalization_data_len,
		       net_app_cert_cb_t cert_cb,
		       net_app_entropy_src_cb_t entropy_src_cb,
		       struct k_mem_pool *pool,
		       k_thread_stack_t stack,
		       size_t stack_size)
{
	if (!request_buf || request_buf_len == 0) {
		NET_ERR("Request buf must be set");
		return -EINVAL;
	}

	/* mbedtls cannot receive or send larger buffer as what is defined
	 * in a file pointed by CONFIG_MBEDTLS_CFG_FILE.
	 */
	if (request_buf_len > MBEDTLS_SSL_MAX_CONTENT_LEN) {
		NET_ERR("Request buf too large, max len is %d",
			MBEDTLS_SSL_MAX_CONTENT_LEN);
		return -EINVAL;
	}

	if (!cert_cb) {
		NET_ERR("Cert callback must be set");
		return -EINVAL;
	}

	if (server_banner) {
		new_server(ctx, server_banner);
	}

	ctx->tls.request_buf = request_buf;
	ctx->tls.request_buf_len = request_buf_len;
	ctx->is_tls = true;
	ctx->tls.stack = stack;
	ctx->tls.stack_size = stack_size;
	ctx->tls.mbedtls.cert_cb = cert_cb;
	ctx->tls.pool = pool;

	if (entropy_src_cb) {
		ctx->tls.mbedtls.entropy_src_cb = entropy_src_cb;
	} else {
		ctx->tls.mbedtls.entropy_src_cb = _net_app_entropy_source;
	}

	ctx->tls.mbedtls.personalization_data = personalization_data;
	ctx->tls.mbedtls.personalization_data_len = personalization_data_len;
	ctx->send_data = _net_app_tls_sendto;
	ctx->recv_cb = _net_app_tls_received;

	/* Then mbedtls specific initialization */
	return 0;
}
#endif /* CONFIG_NET_APP_TLS */
