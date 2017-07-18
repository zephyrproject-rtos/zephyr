/* client.c */

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
#include <net/dns_resolve.h>

#include <net/net_app.h>

#include "../../ip/udp_internal.h"

#include "net_app_private.h"

#if defined(CONFIG_NET_APP_TLS)
#define TLS_STARTUP_TIMEOUT K_SECONDS(5)
static int start_tls_client(struct net_app_ctx *ctx);
#endif /* CONFIG_NET_APP_TLS */

#if defined(CONFIG_DNS_RESOLVER)
static void dns_cb(enum dns_resolve_status status,
		   struct dns_addrinfo *info,
		   void *user_data)
{
	struct net_app_ctx *ctx = user_data;

	if (!(status == DNS_EAI_INPROGRESS && info)) {
		return;
	}

	if (info->ai_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		net_ipaddr_copy(&net_sin(&ctx->ipv4.remote)->sin_addr,
				&net_sin(&info->ai_addr)->sin_addr);
		ctx->ipv4.remote.family = info->ai_family;
#else
		goto out;
#endif
	} else if (info->ai_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		net_ipaddr_copy(&net_sin6(&ctx->ipv6.remote)->sin6_addr,
				&net_sin6(&info->ai_addr)->sin6_addr);
		ctx->ipv6.remote.family = info->ai_family;
#else
		goto out;
#endif
	} else {
		goto out;
	}

out:
	k_sem_give(&ctx->client.dns_wait);
}

static int resolve_name(struct net_app_ctx *ctx,
			const char *peer_addr_str,
			enum dns_query_type type,
			s32_t timeout)
{
	int ret;

	k_sem_init(&ctx->client.dns_wait, 0, 1);

	ret = dns_get_addr_info(peer_addr_str, type, &ctx->client.dns_id,
				dns_cb, ctx, timeout);
	if (ret < 0) {
		NET_ERR("Cannot resolve %s (%d)", peer_addr_str, ret);
		ctx->client.dns_id = 0;
		return ret;
	}

	/* Wait a little longer for the DNS to finish so that
	 * the DNS will timeout before the semaphore.
	 */
	if (k_sem_take(&ctx->client.dns_wait, timeout + K_SECONDS(1))) {
		NET_ERR("Timeout while resolving %s", peer_addr_str);
		ctx->client.dns_id = 0;
		return -ETIMEDOUT;
	}

	ctx->client.dns_id = 0;

	if (ctx->default_ctx->remote.family == AF_UNSPEC) {
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_DNS_RESOLVER */

static int get_port_number(const char *peer_addr_str,
			   char *buf,
			   size_t buf_len)
{
	u16_t port = 0;
	char *ptr;
	int count, i;

	if (peer_addr_str[0] == '[') {
#if defined(CONFIG_NET_IPV6)
		/* IPv6 address with port number */
		int end;

		ptr = strstr(peer_addr_str, "]:");
		if (!ptr) {
			return -EINVAL;
		}

		end = min(INET6_ADDRSTRLEN, ptr - (peer_addr_str + 1));
		memcpy(buf, peer_addr_str + 1, end);
		buf[end] = '\0';

		port = strtol(ptr + 2, NULL, 10);

		return port;
#else
		return -EAFNOSUPPORT;
#endif /* CONFIG_NET_IPV6 */
	}

	count = i = 0;
	while (peer_addr_str[i]) {
		if (peer_addr_str[i] == ':') {
			count++;
		}

		i++;
	}

	if (count == 1) {
#if defined(CONFIG_NET_IPV4)
		/* IPv4 address with port number */
		int end;

		ptr = strstr(peer_addr_str, ":");
		if (!ptr) {
			return -EINVAL;
		}

		end = min(NET_IPV4_ADDR_LEN, ptr - peer_addr_str);
		memcpy(buf, peer_addr_str, end);
		buf[end] = '\0';

		port = strtol(ptr + 1, NULL, 10);

		return port;
#else
		return -EAFNOSUPPORT;
#endif /* CONFIG_NET_IPV4 */
	}

	return 0;
}

static int set_remote_addr(struct net_app_ctx *ctx,
			   const char *peer_addr_str,
			   u16_t peer_port,
			   s32_t timeout)
{
	char addr_str[INET6_ADDRSTRLEN + 1];
	char *addr;
	int ret;

	/* If the peer string contains port number, use that and ignore
	 * the port number parameter.
	 */
	ret = get_port_number(peer_addr_str, addr_str, sizeof(addr_str));
	if (ret > 0) {
		addr = addr_str;
		peer_port = ret;
	} else {
		addr = (char *)peer_addr_str;
	}

#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
	ret = net_addr_pton(AF_INET6, addr,
			    &net_sin6(&ctx->ipv6.remote)->sin6_addr);
	if (ret < 0) {
		/* Could be hostname, try DNS if configured. */
#if !defined(CONFIG_DNS_RESOLVER)
		NET_ERR("Invalid IPv6 address %s", addr);
		return -EINVAL;
#else
		ret = resolve_name(ctx, addr, DNS_QUERY_TYPE_AAAA, timeout);
		if (ret < 0) {
			NET_ERR("Cannot resolve %s (%d)", addr, ret);
			return ret;
		}
#endif
	}

	net_sin6(&ctx->ipv6.remote)->sin6_port = htons(peer_port);
	net_sin6(&ctx->ipv6.remote)->sin6_family = AF_INET6;
	ctx->ipv6.remote.family = AF_INET6;
	ctx->default_ctx = &ctx->ipv6;
#endif /* IPV6 && !IPV4 */

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
	ret = net_addr_pton(AF_INET, addr,
			    &net_sin(&ctx->ipv4.remote)->sin_addr);
	if (ret < 0) {
		/* Could be hostname, try DNS if configured. */
#if !defined(CONFIG_DNS_RESOLVER)
		NET_ERR("Invalid IPv4 address %s", addr);
		return -EINVAL;
#else
		ret = resolve_name(ctx, addr, DNS_QUERY_TYPE_A, timeout);
		if (ret < 0) {
			NET_ERR("Cannot resolve %s (%d)", addr, ret);
			return ret;
		}
#endif
	}

	net_sin(&ctx->ipv4.remote)->sin_port = htons(peer_port);
	net_sin(&ctx->ipv4.remote)->sin_family = AF_INET;
	ctx->ipv4.remote.family = AF_INET;
	ctx->default_ctx = &ctx->ipv4;
#endif /* IPV6 && !IPV4 */

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
	ret = net_addr_pton(AF_INET, addr,
			    &net_sin(&ctx->ipv4.remote)->sin_addr);
	if (ret < 0) {
		ret = net_addr_pton(AF_INET6, addr,
				    &net_sin6(&ctx->ipv6.remote)->sin6_addr);
		if (ret < 0) {
			/* Could be hostname, try DNS if configured. */
#if !defined(CONFIG_DNS_RESOLVER)
			NET_ERR("Invalid IPv4 or IPv6 address %s", addr);
			return -EINVAL;
#else
			ret = resolve_name(ctx, addr,
					   DNS_QUERY_TYPE_A, timeout);
			if (ret < 0) {
				ret = resolve_name(ctx, addr,
						   DNS_QUERY_TYPE_AAAA,
						   timeout);
				if (ret < 0) {
					NET_ERR("Cannot resolve %s (%d)",
						addr, ret);
					return ret;
				}

				goto ipv6;
			}

			goto ipv4;
#endif /* !CONFIG_DNS_RESOLVER */
		} else {
#if defined(CONFIG_DNS_RESOLVER)
		ipv6:
#endif
			net_sin6(&ctx->ipv6.remote)->sin6_port =
				htons(peer_port);
			net_sin6(&ctx->ipv6.remote)->sin6_family = AF_INET6;
			ctx->ipv6.remote.family = AF_INET6;
			ctx->default_ctx = &ctx->ipv6;
		}
	} else {
#if defined(CONFIG_DNS_RESOLVER)
	ipv4:
#endif
		net_sin(&ctx->ipv4.remote)->sin_port = htons(peer_port);
		net_sin(&ctx->ipv4.remote)->sin_family = AF_INET;
		ctx->ipv4.remote.family = AF_INET;
		ctx->default_ctx = &ctx->ipv4;
	}
#endif /* IPV4 && IPV6 */

	/* If we have not yet figured out what is the protocol family,
	 * then we cannot continue.
	 */
	if (!ctx->default_ctx ||
	    ctx->default_ctx->remote.family == AF_UNSPEC) {
		NET_ERR("Unknown protocol family.");
		return -EPFNOSUPPORT;
	}

	return 0;
}

int net_app_init_client(struct net_app_ctx *ctx,
			enum net_sock_type sock_type,
			enum net_ip_protocol proto,
			struct sockaddr *client_addr,
			struct sockaddr *peer_addr,
			const char *peer_addr_str,
			u16_t peer_port,
			s32_t timeout,
			void *user_data)
{
	struct sockaddr addr;
	int ret;

	if (!ctx) {
		return -EINVAL;
	}

	if (ctx->is_init) {
		return -EALREADY;
	}

	memset(&addr, 0, sizeof(addr));

	if (client_addr) {
		memcpy(&addr, client_addr, sizeof(addr));
	} else {
		addr.family = AF_UNSPEC;
	}

	ctx->app_type = NET_APP_CLIENT;
	ctx->user_data = user_data;
	ctx->send_data = net_context_sendto;
	ctx->recv_cb = _net_app_received;
	ctx->proto = proto;
	ctx->sock_type = sock_type;

	ret = _net_app_config_local_ctx(ctx, sock_type, proto, &addr);
	if (ret < 0) {
		goto fail;
	}

	if (peer_addr) {
		if (peer_addr->family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
			memcpy(&ctx->ipv4.remote, peer_addr,
			       sizeof(ctx->ipv4.remote));
			ctx->default_ctx = &ctx->ipv4;
#else
			return -EPROTONOSUPPORT;
#endif
		} else if (peer_addr->family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
			memcpy(&ctx->ipv6.remote, peer_addr,
			       sizeof(ctx->ipv6.remote));
			ctx->default_ctx = &ctx->ipv6;
#else
			return -EPROTONOSUPPORT;
#endif
		}

		goto out;
	}

	if (!peer_addr_str) {
		NET_ERR("Cannot know where to connect.");
		ret = -EINVAL;
		goto fail;
	}

	ret = set_remote_addr(ctx, peer_addr_str, peer_port, timeout);
	if (ret < 0) {
		goto fail;
	}

#if defined(CONFIG_NET_IPV4)
	if (ctx->ipv4.remote.family == AF_INET) {
		ctx->ipv4.local.family = AF_INET;
		_net_app_set_local_addr(&ctx->ipv4.local, NULL,
					net_sin(&ctx->ipv4.local)->sin_port);

		ret = _net_app_set_net_ctx(ctx, ctx->ipv4.ctx,
					   &ctx->ipv4.local,
					   sizeof(struct sockaddr_in),
					   ctx->proto);
		if (ret < 0) {
			net_context_put(ctx->ipv4.ctx);
			ctx->ipv4.ctx = NULL;
		}
	}
#endif

#if defined(CONFIG_NET_IPV6)
	if (ctx->ipv6.remote.family == AF_INET6) {
		ctx->ipv6.local.family = AF_INET6;
		_net_app_set_local_addr(&ctx->ipv6.local, NULL,
				       net_sin6(&ctx->ipv6.local)->sin6_port);

		ret = _net_app_set_net_ctx(ctx, ctx->ipv6.ctx,
					   &ctx->ipv6.local,
					   sizeof(struct sockaddr_in6),
					   ctx->proto);
		if (ret < 0) {
			net_context_put(ctx->ipv6.ctx);
			ctx->ipv6.ctx = NULL;
		}
	}
#endif

	_net_app_print_info(ctx);

out:
	ctx->is_init = true;

fail:
	return ret;

}

static void _app_connected(struct net_context *net_ctx,
			   int status,
			   void *user_data)
{
	struct net_app_ctx *ctx = user_data;

#if defined(CONFIG_NET_APP_TLS)
	if (ctx->is_tls) {
		k_sem_give(&ctx->client.connect_wait);
	}
#endif

	net_context_recv(net_ctx, ctx->recv_cb, K_NO_WAIT, ctx);

#if defined(CONFIG_NET_APP_TLS)
	if (ctx->is_tls) {
		/* If we have TLS connection, the connect cb is called
		 * after TLS handshakes are done.
		 */
		NET_DBG("Postponing TLS connection cb for ctx %p", ctx);
	} else
#endif
	{
		if (ctx->cb.connect) {
			ctx->cb.connect(ctx, status, ctx->user_data);
		}
	}
}

#if defined(CONFIG_NET_APP_DTLS)
static int connect_dtls(struct net_app_ctx *ctx, struct net_context *orig,
			struct sockaddr *remote)
{
	struct net_context *dtls_context;
	struct sockaddr local_addr;
	int ret;

	/* We create a new context that starts to send data and get replies
	 * directly into correct callback.
	 */
	ret = net_context_get(net_context_get_family(orig), SOCK_DGRAM,
			      IPPROTO_UDP, &dtls_context);
	if (ret < 0) {
		NET_DBG("Cannot get connect context");
		goto out;
	}

	memcpy(&dtls_context->remote, remote, sizeof(dtls_context->remote));

#if defined(CONFIG_NET_IPV6)
	if (net_context_get_family(orig) == AF_INET6) {
		struct sockaddr_in6 *local_addr6 = net_sin6(&local_addr);

		net_sin6(&dtls_context->remote)->sin6_family = AF_INET6;

		local_addr6->sin6_family = AF_INET6;
		local_addr6->sin6_port = net_sin6_ptr(&orig->local)->sin6_port;
		net_ipaddr_copy(&local_addr6->sin6_addr,
				net_sin6_ptr(&orig->local)->sin6_addr);
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_context_get_family(orig) == AF_INET) {
		struct sockaddr_in *local_addr4 = net_sin(&local_addr);

		net_sin(&dtls_context->remote)->sin_family = AF_INET;

		local_addr4->sin_family = AF_INET;
		local_addr4->sin_port = net_sin_ptr(&orig->local)->sin_port;
		net_ipaddr_copy(&local_addr4->sin_addr,
				net_sin_ptr(&orig->local)->sin_addr);
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		NET_ASSERT_INFO(false, "Invalid protocol family %d",
				net_context_get_family(orig));
		goto ctx_drop;
	}

	ret = net_context_bind(dtls_context, &local_addr, sizeof(local_addr));
	if (ret < 0) {
		NET_DBG("Cannot bind connect DTLS context");
		goto ctx_drop;
	}

	dtls_context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;

	ret = net_udp_register(&dtls_context->remote,
			       &local_addr,
			       ntohs(net_sin(&dtls_context->remote)->sin_port),
			       ntohs(net_sin(&local_addr)->sin_port),
			       _net_app_dtls_established,
			       ctx,
			       &dtls_context->conn_handler);
	if (ret < 0) {
		NET_DBG("Cannot register connect DTLS handler (%d)", ret);
		goto ctx_drop;
	}

	NET_DBG("New DTLS connection context %p created", dtls_context);

	ctx->dtls.ctx = dtls_context;

	return 0;

ctx_drop:
	net_context_unref(dtls_context);

out:
	return -ECONNABORTED;
}
#endif /* CONFIG_NET_APP_DTLS */

int net_app_connect(struct net_app_ctx *ctx, s32_t timeout)
{
	struct net_context *net_ctx;
	bool started = false;
	int ret;

	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	if (ctx->app_type != NET_APP_CLIENT) {
		return -EINVAL;
	}

	net_ctx = _net_app_select_net_ctx(ctx, NULL);
	if (!net_ctx) {
		return -EAFNOSUPPORT;
	}

#if defined(CONFIG_NET_APP_TLS)
	if (ctx->is_tls && !ctx->tls.tid &&
	    (ctx->proto == IPPROTO_TCP ||
	     (IS_ENABLED(CONFIG_NET_APP_DTLS) && ctx->proto == IPPROTO_UDP))) {
		/* TLS thread is not yet running, start it now */
		ret = start_tls_client(ctx);
		if (ret < 0) {
			NET_DBG("TLS thread cannot be started (%d)", ret);
			return ret;
		}

		started = true;

		/* Let the TLS thread run first */
		k_yield();
	}
#else
	ARG_UNUSED(started);
#endif /* CONFIG_NET_APP_TLS */

#if defined(CONFIG_NET_APP_DTLS)
	if (ctx->proto == IPPROTO_UDP) {
		if (!ctx->dtls.ctx) {
			ret = connect_dtls(ctx, net_ctx,
					   &ctx->default_ctx->remote);
			if (ret < 0) {
				return ret;
			}

			ret = net_context_connect(ctx->dtls.ctx,
						  &ctx->dtls.ctx->remote,
						  sizeof(ctx->dtls.ctx->remote),
						  _app_connected,
						  timeout,
						  ctx);
		} else {
			/* If we have already a connection, then we cannot
			 * really continue.
			 */
			ret = -EAGAIN;
		}
	} else
#endif /* CONFIG_NET_APP_DTLS */
	{
		ret = net_context_connect(net_ctx,
					  &ctx->default_ctx->remote,
					  sizeof(ctx->default_ctx->remote),
					  _app_connected,
					  timeout,
					  ctx);
	}

	if (ret < 0) {
		NET_DBG("Cannot connect to peer (%d)", ret);

#if defined(CONFIG_NET_APP_TLS)
		if (started) {
			_net_app_tls_handler_stop(ctx);
		}
#endif
	}

	return ret;
}

#if defined(CONFIG_NET_APP_TLS)
static void tls_client_handler(struct net_app_ctx *ctx,
			       struct k_sem *startup_sync)
{
	int ret;

	NET_DBG("Starting TLS client thread for %p", ctx);

	ret = _net_app_tls_init(ctx, MBEDTLS_SSL_IS_CLIENT);
	if (ret < 0) {
		NET_DBG("TLS client init failed");
		return;
	}

	k_sem_give(startup_sync);

	/* We wait until TLS connection is established */
	k_sem_take(&ctx->client.connect_wait, K_FOREVER);

	ret = _net_app_ssl_mainloop(ctx);
	if (ret < 0) {
		NET_ERR("TLS mainloop startup failed (%d)", ret);
	}

	mbedtls_ssl_close_notify(&ctx->tls.mbedtls.ssl);

	/* If there is any pending data that have not been processed
	 * yet, we need to free it here.
	 */
	if (ctx->tls.mbedtls.ssl_ctx.rx_pkt) {
		net_pkt_unref(ctx->tls.mbedtls.ssl_ctx.rx_pkt);
		ctx->tls.mbedtls.ssl_ctx.rx_pkt = NULL;
		ctx->tls.mbedtls.ssl_ctx.frag = NULL;
	}

	if (ctx->cb.close) {
		ctx->cb.close(ctx, -ESHUTDOWN, ctx->user_data);
	}

	_net_app_tls_handler_stop(ctx);
}

static int start_tls_client(struct net_app_ctx *ctx)
{
	struct k_sem startup_sync;

	/* Start the thread that handles TLS traffic. */
	if (ctx->tls.tid) {
		return -EALREADY;
	}

	k_sem_init(&startup_sync, 0, 1);

	ctx->tls.tid = k_thread_create(&ctx->tls.thread,
				       ctx->tls.stack,
				       ctx->tls.stack_size,
				       (k_thread_entry_t)tls_client_handler,
				       ctx, &startup_sync, 0,
				       K_PRIO_COOP(7), 0, 0);

	/* Wait until we know that the TLS thread startup was ok */
	if (k_sem_take(&startup_sync, TLS_STARTUP_TIMEOUT) < 0) {
		_net_app_tls_handler_stop(ctx);
		return -ECANCELED;
	}

	return 0;
}

int net_app_client_tls(struct net_app_ctx *ctx,
		       u8_t *request_buf,
		       size_t request_buf_len,
		       u8_t *personalization_data,
		       size_t personalization_data_len,
		       net_app_ca_cert_cb_t cert_cb,
		       const char *cert_host,
		       net_app_entropy_src_cb_t entropy_src_cb,
		       struct k_mem_pool *pool,
		       u8_t *stack,
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

	ctx->is_tls = true;
	ctx->send_data = _net_app_tls_sendto;
	ctx->recv_cb = _net_app_tls_received;
	ctx->tls.request_buf = request_buf;
	ctx->tls.request_buf_len = request_buf_len;
	ctx->tls.cert_host = cert_host;
	ctx->tls.stack = stack;
	ctx->tls.stack_size = stack_size;
	ctx->tls.mbedtls.ca_cert_cb = cert_cb;
	ctx->tls.pool = pool;
	ctx->tls.mbedtls.personalization_data = personalization_data;
	ctx->tls.mbedtls.personalization_data_len = personalization_data_len;

	if (entropy_src_cb) {
		ctx->tls.mbedtls.entropy_src_cb = entropy_src_cb;
	} else {
		ctx->tls.mbedtls.entropy_src_cb = _net_app_entropy_source;
	}

	/* The semaphore is released when the client calls net_app_connect() */
	k_sem_init(&ctx->client.connect_wait, 0, 1);

	/* The mbedtls is initialized in TLS thread because of mbedtls stack
	 * requirements. TLS thread is started when we get the first client
	 * request to send data.
	 */
	return 0;
}
#endif /* CONFIG_NET_APP_TLS */
