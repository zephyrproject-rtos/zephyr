/* net_app.c */

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
#include <net/dhcpv4.h>
#include <net/net_mgmt.h>

#include <net/net_app.h>

#include "net_app_private.h"

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
int net_app_set_net_pkt_pool(struct net_app_ctx *ctx,
			     net_pkt_get_slab_func_t tx_slab,
			     net_pkt_get_pool_func_t data_pool)
{
	ctx->tx_slab = tx_slab;
	ctx->data_pool = data_pool;

	return 0;
}
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

#if defined(CONFIG_NET_DEBUG_APP)
char *_net_app_sprint_ipaddr(char *buf, int buflen,
			     const struct sockaddr *addr)
{
	if (addr->family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		char ipaddr[NET_IPV6_ADDR_LEN];

		net_addr_ntop(addr->family,
			      &net_sin6(addr)->sin6_addr,
			      ipaddr, sizeof(ipaddr));
		snprintk(buf, buflen, "[%s]:%u", ipaddr,
			 ntohs(net_sin6(addr)->sin6_port));
#endif
	} else if (addr->family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		char ipaddr[NET_IPV4_ADDR_LEN];

		net_addr_ntop(addr->family,
			      &net_sin(addr)->sin_addr,
			      ipaddr, sizeof(ipaddr));
		snprintk(buf, buflen, "%s:%u", ipaddr,
			 ntohs(net_sin(addr)->sin_port));
#endif
	}

	return buf;
}
#endif /* CONFIG_NET_DEBUG_APP */

#if defined(CONFIG_NET_APP_SERVER) || defined(CONFIG_NET_APP_CLIENT)
void _net_app_received(struct net_context *net_ctx,
		       struct net_pkt *pkt,
		       int status,
		       void *user_data)
{
	struct net_app_ctx *ctx = user_data;

#if defined(CONFIG_NET_APP_CLIENT)
	if (ctx->app_type == NET_APP_CLIENT) {
		if (!pkt) {
			if (ctx->cb.close) {
				ctx->cb.close(ctx, status, ctx->user_data);
			}

			return;
		}

		if (ctx->cb.recv) {
			ctx->cb.recv(ctx, pkt, status, ctx->user_data);
		}
	}
#endif

#if defined(CONFIG_NET_APP_SERVER)
	if (ctx->app_type == NET_APP_SERVER) {
		if (!pkt) {
			if (ctx->cb.close) {
				ctx->cb.close(ctx, status, ctx->user_data);
			}

			if (ctx->proto == IPPROTO_TCP) {
				net_context_put(ctx->server.net_ctx);
				ctx->server.net_ctx = NULL;
			}

			return;
		}

		if (ctx->cb.recv) {
			ctx->cb.recv(ctx, pkt, status, ctx->user_data);
		}
	}
#endif
}
#endif /* CONFIG_NET_APP_SERVER || CONFIG_NET_APP_CLIENT */

#if defined(CONFIG_NET_APP_SERVER) || defined(CONFIG_NET_APP_CLIENT)
int _net_app_set_net_ctx(struct net_app_ctx *ctx,
			 struct net_context *net_ctx,
			 struct sockaddr *addr,
			 socklen_t socklen,
			 enum net_ip_protocol proto)
{
	int ret;

	ret = net_context_bind(net_ctx, addr, socklen);
	if (ret < 0) {
		NET_ERR("Cannot bind context (%d)", ret);
		goto out;
	}

#if defined(CONFIG_NET_APP_SERVER) && defined(CONFIG_NET_TCP)
	if (ctx->app_type == NET_APP_SERVER && proto == IPPROTO_TCP) {
		ret = net_context_listen(net_ctx, 0);
		if (ret < 0) {
			NET_ERR("Cannot listen context (%d)", ret);
			goto out;
		}

		ret = net_context_accept(net_ctx, _net_app_accept_cb,
					 K_NO_WAIT, ctx);
		if (ret < 0) {
			NET_ERR("Cannot accept context (%d)", ret);
			goto out;
		}

		/* TCP recv callback is set after we have accepted the
		 * connection.
		 */
	}
#endif /* CONFIG_NET_APP_SERVER && CONFIG_NET_TCP */

#if defined(CONFIG_NET_APP_SERVER) && defined(CONFIG_NET_UDP)
	if (ctx->app_type == NET_APP_SERVER && proto == IPPROTO_UDP) {
		net_context_recv(net_ctx, _net_app_received, K_NO_WAIT, ctx);
	}
#endif /* CONFIG_NET_APP_SERVER && CONFIG_NET_UDP */

out:
	return ret;
}

int _net_app_set_local_addr(struct sockaddr *addr, const char *myaddr,
			    u16_t port)
{
	if (myaddr) {
		void *inaddr;

		if (addr->family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
			inaddr = &net_sin(addr)->sin_addr;
			net_sin(addr)->sin_port = htons(port);
#else
			return -EPFNOSUPPORT;
#endif
		} else if (addr->family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
			inaddr = &net_sin6(addr)->sin6_addr;
			net_sin6(addr)->sin6_port = htons(port);
#else
			return -EPFNOSUPPORT;
#endif
		} else {
			return -EAFNOSUPPORT;
		}

		return net_addr_pton(addr->family, myaddr, inaddr);
	}

	/* If the caller did not supply the address where to bind, then
	 * try to figure it out ourselves.
	 */
	if (addr->family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		net_ipaddr_copy(&net_sin6(addr)->sin6_addr,
				net_if_ipv6_select_src_addr(NULL,
					(struct in6_addr *)
					net_ipv6_unspecified_address()));
#else
		return -EPFNOSUPPORT;
#endif
	} else if (addr->family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		struct net_if *iface = net_if_get_default();

		/* For IPv4 we take the first address in the interface */
		net_ipaddr_copy(&net_sin(addr)->sin_addr,
				&iface->ipv4.unicast[0].address.in_addr);
#else
		return -EPFNOSUPPORT;
#endif
	}

	return 0;
}
#endif /* CONFIG_NET_APP_SERVER || CONFIG_NET_APP_CLIENT */

#if defined(CONFIG_NET_APP_SERVER)
#endif /* CONFIG_NET_APP_SERVER */

#if defined(CONFIG_NET_IPV4) && (defined(CONFIG_NET_APP_SERVER) || \
				 defined(CONFIG_NET_APP_CLIENT))
static int setup_ipv4_ctx(struct net_app_ctx *ctx,
			  enum net_sock_type sock_type,
			  enum net_ip_protocol proto)
{
	int ret;

	ret = net_context_get(AF_INET, sock_type, proto, &ctx->ipv4_ctx);
	if (ret < 0) {
		NET_ERR("Cannot get network context (%d)", ret);
		ctx->ipv4_ctx = NULL;
		return ret;
	}

	net_context_setup_pools(ctx->ipv4_ctx, ctx->tx_slab,
				ctx->data_pool);

	return ret;
}
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_IPV6) && (defined(CONFIG_NET_APP_SERVER) || \
				 defined(CONFIG_NET_APP_CLIENT))
static int setup_ipv6_ctx(struct net_app_ctx *ctx,
			  enum net_sock_type sock_type,
			  enum net_ip_protocol proto)
{
	int ret;

	ret = net_context_get(AF_INET6, sock_type, proto, &ctx->ipv6_ctx);
	if (ret < 0) {
		NET_ERR("Cannot get network context (%d)", ret);
		ctx->ipv6_ctx = NULL;
		return ret;
	}

	net_context_setup_pools(ctx->ipv6_ctx, ctx->tx_slab,
				ctx->data_pool);

	return ret;
}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_APP_SERVER) || defined(CONFIG_NET_APP_CLIENT)
int _net_app_config_local_ctx(struct net_app_ctx *ctx,
			      enum net_sock_type sock_type,
			      enum net_ip_protocol proto,
			      struct sockaddr *addr)
{
	int ret;

	if (addr->family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		ret = setup_ipv6_ctx(ctx, sock_type, proto);
#else
		ret = -EPFNOSUPPORT;
		goto fail;
#endif
	} else if (addr->family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		ret = setup_ipv4_ctx(ctx, sock_type, proto);
#else
		ret = -EPFNOSUPPORT;
		goto fail;
#endif
	} else if (addr->family == AF_UNSPEC) {
#if defined(CONFIG_NET_IPV4)
		ret = setup_ipv4_ctx(ctx, sock_type, proto);
#endif
		/* We ignore the IPv4 error if IPv6 is enabled */
#if defined(CONFIG_NET_IPV6)
		ret = setup_ipv6_ctx(ctx, sock_type, proto);
#endif
	} else {
		ret = -EINVAL;
		goto fail;
	}

fail:
	return ret;
}
#endif /* CONFIG_NET_APP_SERVER || CONFIG_NET_APP_CLIENT */

int net_app_release(struct net_app_ctx *ctx)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

#if defined(CONFIG_NET_IPV6)
	if (ctx->ipv6_ctx) {
		net_context_put(ctx->ipv6_ctx);
		ctx->ipv6_ctx = NULL;
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (ctx->ipv4_ctx) {
		net_context_put(ctx->ipv4_ctx);
		ctx->ipv4_ctx = NULL;
	}
#endif /* CONFIG_NET_IPV4 */

	ctx->is_init = false;

	return 0;
}

struct net_context *_net_app_select_net_ctx(struct net_app_ctx *ctx)
{
#if defined(CONFIG_NET_APP_CLIENT)
	if (ctx->app_type == NET_APP_CLIENT) {
		if (ctx->remote.family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
			return ctx->ipv6_ctx;
#endif
		} else if (ctx->remote.family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
			return ctx->ipv4_ctx;
#endif
		}
	}
#endif /* CONFIG_NET_APP_CLIENT */

#if defined(CONFIG_NET_APP_SERVER)
	if (ctx->app_type == NET_APP_SERVER) {
		if (ctx->proto == IPPROTO_TCP) {
			return ctx->server.net_ctx;
		} else if (ctx->proto == IPPROTO_UDP) {
			if (ctx->local.family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
				return ctx->ipv6_ctx;
#endif
			} else if (ctx->local.family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
				return ctx->ipv4_ctx;
#endif
			}
		}
	}
#endif /* CONFIG_NET_APP_SERVER */

	return NULL;
}

int net_app_set_cb(struct net_app_ctx *ctx,
		   net_app_connect_cb_t connect_cb,
		   net_app_recv_cb_t recv_cb,
		   net_app_send_cb_t send_cb,
		   net_app_close_cb_t close_cb)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	ctx->cb.connect = connect_cb;
	ctx->cb.recv = recv_cb;
	ctx->cb.send = send_cb;
	ctx->cb.close = close_cb;

	return 0;
}

static void _app_send(struct net_context *net_ctx,
		      int status,
		      void *token,
		      void *user_data)
{
	struct net_app_ctx *ctx = user_data;

	ARG_UNUSED(ctx);

#if defined(CONFIG_NET_APP_CLIENT)
	if (ctx->app_type == NET_APP_CLIENT && ctx->cb.send) {
		ctx->cb.send(ctx, status, token, ctx->user_data);
	}
#endif

#if defined(CONFIG_NET_APP_SERVER)
	if (ctx->app_type == NET_APP_SERVER && ctx->cb.send) {
		ctx->cb.send(ctx, status, token, ctx->user_data);
	}
#endif
}

int net_app_send_pkt(struct net_app_ctx *ctx,
		     struct net_pkt *pkt,
		     const struct sockaddr *dst,
		     socklen_t dst_len,
		     s32_t timeout,
		     void *user_data_send)
{
	int ret;

	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	net_pkt_set_appdatalen(pkt, net_buf_frags_len(pkt->frags));

	if (!dst && ctx->proto == IPPROTO_UDP) {
		dst = &ctx->remote;

		if (ctx->remote.family == AF_INET) {
			dst_len = sizeof(struct sockaddr_in);
		} else {
			dst_len = sizeof(struct sockaddr_in6);
		}
	}

	ret = ctx->send_data(pkt, dst, dst_len, _app_send, timeout,
			     user_data_send, ctx);
	if (ret < 0) {
		NET_DBG("Cannot send to peer (%d)", ret);
	}

	return ret;
}

int net_app_send_buf(struct net_app_ctx *ctx,
		     u8_t *buf,
		     size_t buf_len,
		     const struct sockaddr *dst,
		     socklen_t dst_len,
		     s32_t timeout,
		     void *user_data_send)
{
	struct net_context *net_ctx;
	struct net_pkt *pkt;
	struct net_buf *frag;
	size_t len, pos = 0;
	int ret;

	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	if (!buf_len) {
		return -EMSGSIZE;
	}

	net_ctx = _net_app_select_net_ctx(ctx);
	if (!net_ctx) {
		return -ENOENT;
	}

	pkt = net_pkt_get_tx(net_ctx, timeout);
	if (!pkt) {
		return -ENOMEM;
	}

	net_pkt_set_appdatalen(pkt, buf_len);

	while (buf_len) {
		frag = net_pkt_get_data(net_ctx, timeout);
		if (!frag) {
			net_pkt_unref(pkt);
			return -ENOMEM;
		}

		len = net_buf_tailroom(frag);
		if (len >= buf_len) {
			net_buf_add_mem(frag, buf + pos, buf_len);
			net_pkt_frag_add(pkt, frag);
			goto send;
		}

		net_buf_add_mem(frag, buf + pos, len);
		net_pkt_frag_add(pkt, frag);

		pos += len;
		buf_len -= len;
	}

send:
	ret = ctx->send_data(pkt, dst, dst_len, _app_send, timeout,
			     user_data_send, ctx);
	if (ret < 0) {
		NET_DBG("Cannot send to peer (%d)", ret);
		net_pkt_unref(pkt);
	}

	return ret;
}

struct net_pkt *net_app_get_net_pkt(struct net_app_ctx *ctx,
				    s32_t timeout)
{
	struct net_context *net_ctx;

	if (!ctx) {
		return NULL;
	}

	if (!ctx->is_init) {
		return NULL;
	}

	net_ctx = _net_app_select_net_ctx(ctx);
	if (!net_ctx) {
		return NULL;
	}

	return net_pkt_get_tx(net_ctx, timeout);
}

struct net_buf *net_app_get_net_buf(struct net_app_ctx *ctx,
				    struct net_pkt *pkt,
				    s32_t timeout)
{
	if (!ctx || !pkt) {
		return NULL;
	}

	if (!ctx->is_init) {
		return NULL;
	}

	return net_pkt_get_frag(pkt, timeout);
}

int net_app_close(struct net_app_ctx *ctx)
{
	struct net_context *net_ctx;

	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	net_ctx = _net_app_select_net_ctx(ctx);
	if (!net_ctx) {
		return -EAFNOSUPPORT;
	}

	if (ctx->cb.close) {
		ctx->cb.close(ctx, 0, ctx->user_data);
	}

#if defined(CONFIG_NET_APP_SERVER)
	if (ctx->app_type == NET_APP_SERVER) {
		ctx->server.net_ctx = NULL;
	}
#endif

	net_context_put(net_ctx);

	return 0;
}

#if defined(CONFIG_NET_APP_TLS)
#if defined(MBEDTLS_DEBUG_C) && defined(CONFIG_NET_DEBUG_APP)
static void my_debug(void *ctx, int level,
		     const char *file, int line, const char *str)
{
	const char *p, *basename;
	int len;

	ARG_UNUSED(ctx);

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}

	}

	/* Avoid printing double newlines */
	len = strlen(str);
	if (str[len - 1] == '\n') {
		((char *)str)[len - 1] = '\0';
	}

	NET_DBG("%s:%04d: |%d| %s", basename, line, level, str);
}
#endif /* MBEDTLS_DEBUG_C && CONFIG_NET_DEBUG_APP */

static void ssl_sent(struct net_context *context,
		     int status, void *token, void *user_data)
{
	struct net_app_ctx *ctx = user_data;

	k_sem_give(&ctx->tls.mbedtls.ssl_ctx.tx_sem);
}

/* Send encrypted data */
int _net_app_ssl_tx(void *context, const unsigned char *buf, size_t size)
{
	struct net_app_ctx *ctx = context;
	struct net_pkt *send_buf;
	int ret, len;

	send_buf = net_app_get_net_pkt(ctx, BUF_ALLOC_TIMEOUT);
	if (!send_buf) {
		return MBEDTLS_ERR_SSL_ALLOC_FAILED;
	}

	ret = net_pkt_append_all(send_buf, size, (u8_t *)buf,
				 BUF_ALLOC_TIMEOUT);
	if (!ret) {
		/* Cannot append data */
		net_pkt_unref(send_buf);
		return 0;
	}

	len = size;

	ret = net_context_send(send_buf, ssl_sent, K_NO_WAIT, NULL, ctx);
	if (ret < 0) {
		net_pkt_unref(send_buf);
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}

	k_sem_take(&ctx->tls.mbedtls.ssl_ctx.tx_sem, K_FOREVER);

	return len;
}

/* This gets plain data and then it passes it to TLS handler thread to be
 * encrypted and transmitted to peer. Note that we do not send the data
 * directly here because of the mbedtls stack requirements which are quite
 * high. So no calls to mbedtls from this processing context.
 */
int _net_app_tls_sendto(struct net_pkt *pkt,
			const struct sockaddr *dst_addr,
			socklen_t addrlen,
			net_context_send_cb_t cb,
			s32_t timeout,
			void *token,
			void *user_data)
{
	struct net_app_ctx *ctx = user_data;
	struct net_app_fifo_block *tx_data;
	struct k_mem_block block;
	int ret;

	ARG_UNUSED(dst_addr);
	ARG_UNUSED(addrlen);

	if (pkt && !net_pkt_appdatalen(pkt)) {
		return -EINVAL;
	}

	ret = k_mem_pool_alloc(ctx->tls.pool, &block,
			       sizeof(struct net_app_fifo_block),
			       BUF_ALLOC_TIMEOUT);
	if (ret < 0) {
		return -ENOMEM;
	}

	tx_data = block.data;
	tx_data->pkt = pkt;
	tx_data->dir = NET_APP_PKT_TX;
	tx_data->token = token;
	tx_data->cb = cb;

	/* For freeing memory later */
	memcpy(&tx_data->block, &block, sizeof(struct k_mem_block));

	k_fifo_put(&ctx->tls.mbedtls.ssl_ctx.tx_rx_fifo, (void *)tx_data);

	return 0;
}

/* Receive encrypted data from network. Put that data into fifo
 * that will be read by tls thread.
 */
void _net_app_tls_received(struct net_context *context,
			   struct net_pkt *pkt,
			   int status,
			   void *user_data)
{
	struct net_app_ctx *ctx = user_data;
	struct net_app_fifo_block *rx_data = NULL;
	struct k_mem_block block;
	int ret;

	ARG_UNUSED(context);
	ARG_UNUSED(status);

	if (pkt && !net_pkt_appdatalen(pkt)) {
		net_pkt_unref(pkt);
		return;
	}

	ret = k_mem_pool_alloc(ctx->tls.pool, &block,
			       sizeof(struct net_app_fifo_block),
			       BUF_ALLOC_TIMEOUT);
	if (ret < 0) {
		if (pkt) {
			net_pkt_unref(pkt);
		}

		NET_DBG("Not enough space in TLS mem pool");
		return;
	}

	rx_data = block.data;
	rx_data->pkt = pkt;
	rx_data->dir = NET_APP_PKT_RX;

	/* For freeing memory later */
	memcpy(&rx_data->block, &block, sizeof(struct k_mem_block));

	k_fifo_put(&ctx->tls.mbedtls.ssl_ctx.tx_rx_fifo, (void *)rx_data);
}

static int tls_sendto(struct net_app_ctx *ctx,
		      struct net_app_fifo_block *tx_data)
{
	u16_t len;
	int ret;

	len = net_pkt_appdatalen(tx_data->pkt);
	if (len == 0) {
		ret = -EINVAL;
		goto out;
	}

	ret = net_frag_linearize(ctx->tls.request_buf,
				 ctx->tls.request_buf_len,
				 tx_data->pkt,
				 net_pkt_ip_hdr_len(tx_data->pkt),
				 len);
	if (ret < 0) {
		NET_DBG("Cannot linearize send data (%d)", ret);
		goto out;
	}

	if (ret != len) {
		NET_DBG("Linear copy error (%u vs %d)", len, ret);
		ret = -EINVAL;
		goto out;
	}

	do {
		ret = mbedtls_ssl_write(&ctx->tls.mbedtls.ssl,
					ctx->tls.request_buf, len);
		if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
			_net_app_print_error(
				"peer closed the connection -0x%x", ret);
			goto out;
		}

		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
		    ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			if (ret < 0) {
				_net_app_print_error(
					"mbedtls_ssl_write returned -0x%x",
					ret);
				goto out;
			}
		}
	} while (ret <= 0);

out:
	if (tx_data->cb) {
		tx_data->cb(net_pkt_context(tx_data->pkt), ret,
			    tx_data->token, ctx);
	}

	net_pkt_unref(tx_data->pkt);

	return ret;
}

/* This will copy data from received net_pkt buf into mbedtls internal buffers.
 */
int _net_app_ssl_mux(void *context, unsigned char *buf, size_t size)
{
	struct net_app_ctx *ctx = context;
	struct net_app_fifo_block *rx_data;
	u16_t read_bytes;
	u8_t *ptr;
	int pos;
	int len;
	int ret = 0;

	if (!ctx->tls.mbedtls.ssl_ctx.frag) {
	again:
		rx_data = k_fifo_get(&ctx->tls.mbedtls.ssl_ctx.tx_rx_fifo,
				     K_FOREVER);
		if (!rx_data->pkt) {
			NET_DBG("Closing %p connection", ctx);
			k_mem_pool_free(&rx_data->block);
			return -EIO;
		}

		/* If the fifo contains something we need to send, then try
		 * to send it here and then go back waiting more data.
		 */
		if (rx_data->dir == NET_APP_PKT_TX) {
			tls_sendto(ctx, rx_data);
			k_mem_pool_free(&rx_data->block);
			goto again;
		}

		ctx->tls.mbedtls.ssl_ctx.rx_pkt = rx_data->pkt;

		k_mem_pool_free(&rx_data->block);

		read_bytes = net_pkt_appdatalen(
					ctx->tls.mbedtls.ssl_ctx.rx_pkt);

		ctx->tls.mbedtls.ssl_ctx.remaining = read_bytes;
		ctx->tls.mbedtls.ssl_ctx.frag =
			ctx->tls.mbedtls.ssl_ctx.rx_pkt->frags;

		ptr = net_pkt_appdata(ctx->tls.mbedtls.ssl_ctx.rx_pkt);
		len = ptr - ctx->tls.mbedtls.ssl_ctx.frag->data;

		if (len > ctx->tls.mbedtls.ssl_ctx.frag->size) {
			NET_ERR("Buf overflow (%d > %u)", len,
				ctx->tls.mbedtls.ssl_ctx.frag->size);
			return -EINVAL;
		}

		/* This will get rid of IP header */
		net_buf_pull(ctx->tls.mbedtls.ssl_ctx.frag, len);
	} else {
		read_bytes = ctx->tls.mbedtls.ssl_ctx.remaining;
		ptr = ctx->tls.mbedtls.ssl_ctx.frag->data;
	}

	len = ctx->tls.mbedtls.ssl_ctx.frag->len;
	pos = 0;
	if (read_bytes > size) {
		while (ctx->tls.mbedtls.ssl_ctx.frag) {
			read_bytes = len < (size - pos) ? len : (size - pos);

#if RX_EXTRA_DEBUG == 1
			NET_DBG("Copying %d bytes", read_bytes);
#endif

			memcpy(buf + pos, ptr, read_bytes);

			pos += read_bytes;
			if (pos < size) {
				ctx->tls.mbedtls.ssl_ctx.frag =
					ctx->tls.mbedtls.ssl_ctx.frag->frags;
				ptr = ctx->tls.mbedtls.ssl_ctx.frag->data;
				len = ctx->tls.mbedtls.ssl_ctx.frag->len;
			} else {
				if (read_bytes == len) {
					ctx->tls.mbedtls.ssl_ctx.frag =
					ctx->tls.mbedtls.ssl_ctx.frag->frags;
				} else {
					net_buf_pull(
					       ctx->tls.mbedtls.ssl_ctx.frag,
					       read_bytes);
				}

				ctx->tls.mbedtls.ssl_ctx.remaining -= size;
				return size;
			}
		}
	} else {
		while (ctx->tls.mbedtls.ssl_ctx.frag) {
#if RX_EXTRA_DEBUG == 1
			NET_DBG("Copying all %d bytes", len);
#endif

			memcpy(buf + pos, ptr, len);

			pos += len;
			ctx->tls.mbedtls.ssl_ctx.frag =
				ctx->tls.mbedtls.ssl_ctx.frag->frags;
			if (!ctx->tls.mbedtls.ssl_ctx.frag) {
				break;
			}

			ptr = ctx->tls.mbedtls.ssl_ctx.frag->data;
			len = ctx->tls.mbedtls.ssl_ctx.frag->len;
		}

		net_pkt_unref(ctx->tls.mbedtls.ssl_ctx.rx_pkt);
		ctx->tls.mbedtls.ssl_ctx.rx_pkt = NULL;
		ctx->tls.mbedtls.ssl_ctx.frag = NULL;
		ctx->tls.mbedtls.ssl_ctx.remaining = 0;

		if (read_bytes != pos) {
			return -EIO;
		}

		ret = read_bytes;
	}

	return ret;
}

int _net_app_entropy_source(void *data, unsigned char *output, size_t len,
			    size_t *olen)
{
	u32_t seed;

	ARG_UNUSED(data);

	seed = sys_rand32_get();

	if (len > sizeof(seed)) {
		len = sizeof(seed);
	}

	memcpy(output, &seed, len);

	*olen = len;
	return 0;
}

#if defined(CONFIG_NET_DEBUG_APP)
void _net_app_print_info(struct net_app_ctx *ctx)
{
#define PORT_STR_LEN sizeof("[]:xxxxx")
	char local[NET_IPV6_ADDR_LEN + PORT_STR_LEN];
	char remote[NET_IPV6_ADDR_LEN + PORT_STR_LEN];

	_net_app_sprint_ipaddr(local, sizeof(local), &ctx->local);
	_net_app_sprint_ipaddr(remote, sizeof(remote), &ctx->remote);

	NET_DBG("net app connect %s %s %s",
		local,
		ctx->app_type == NET_APP_CLIENT ? "->" : "<-",
		remote);
}
#endif

int _net_app_ssl_mainloop(struct net_app_ctx *ctx)
{
	size_t len;
	int ret;

reset:
	mbedtls_ssl_session_reset(&ctx->tls.mbedtls.ssl);
	mbedtls_ssl_set_bio(&ctx->tls.mbedtls.ssl, ctx,
			    _net_app_ssl_tx, _net_app_ssl_mux, NULL);

	/* SSL handshake. The ssl_rx() function will be called next by
	 * mbedtls library. The ssl_rx() will block and wait that data is
	 * received by ssl_received() and passed to it via fifo. After
	 * receiving the data, this function will then proceed with secure
	 * connection establishment.
	 */
	/* Waiting SSL handshake */
	do {
		ret = mbedtls_ssl_handshake(&ctx->tls.mbedtls.ssl);
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
		    ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			if (ret < 0) {
				goto close;
			}
		}
	} while (ret != 0);

	/* We call the connect cb only once for each connection. The TLS
	 * might require new handshakes etc, but application does not need
	 * to care about that.
	 */
	if (!ctx->tls.connect_cb_called && ctx->cb.connect) {
		NET_DBG("Calling connect cb for ctx %p", ctx);
		ctx->cb.connect(ctx, 0, ctx->user_data);
		ctx->tls.connect_cb_called = true;
	}

	do {
	again:
		len = ctx->tls.request_buf_len - 1;
		memset(ctx->tls.request_buf, 0, ctx->tls.request_buf_len);

		ret = mbedtls_ssl_read(&ctx->tls.mbedtls.ssl,
				       ctx->tls.request_buf, len);
		if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		    ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			continue;
		}

		if (ret <= 0) {
			switch (ret) {
			case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
				NET_DBG("Connection was closed gracefully");
				goto close;

			case MBEDTLS_ERR_NET_CONN_RESET:
				NET_DBG("Connection was reset by peer");
				break;

			case -EIO:
				break;

			default:
				_net_app_print_error(
					"mbedtls_ssl_read returned -0x%x",
					ret);
				break;
			}

			goto close;
		}

		if (ctx->cb.recv) {
			struct net_pkt *pkt;
			int len = ret;

			pkt = net_pkt_get_rx(_net_app_select_net_ctx(ctx),
					     BUF_ALLOC_TIMEOUT);
			if (!pkt) {
				ret = -ENOMEM;
				goto close;
			}

			ret = net_pkt_append_all(pkt, len,
						 ctx->tls.request_buf,
						 BUF_ALLOC_TIMEOUT);
			if (!ret) {
				/* Not all data was appended */
				net_pkt_unref(pkt);
				ret = -ENOMEM;
				goto close;
			}

			net_pkt_set_appdatalen(pkt, len);
			net_pkt_set_appdata(pkt, pkt->frags->data);

			ctx->cb.recv(ctx, pkt, 0, ctx->user_data);

			goto again;
		}

	} while (ret < 0);

	/* Read another message */
	goto reset;

close:
	/* The -EIO code means that the connection was closed. The error
	 * value is not known by mbedtls so do not print info about it.
	 */
	if (ret != -EIO) {
		_net_app_print_error("Closing connection -0x%x", ret);
	}

	return ret;
}

int _net_app_tls_init(struct net_app_ctx *ctx, int client_or_server)
{
	int ret;

	k_fifo_init(&ctx->tls.mbedtls.ssl_ctx.tx_rx_fifo);
	k_sem_init(&ctx->tls.mbedtls.ssl_ctx.tx_sem, 0, UINT_MAX);

	mbedtls_platform_set_printf(printk);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	if (client_or_server == MBEDTLS_SSL_IS_SERVER) {
#if defined(CONFIG_NET_APP_SERVER)
		mbedtls_x509_crt_init(&ctx->tls.mbedtls.srvcert);
#endif
	} else {
#if defined(CONFIG_NET_APP_CLIENT)
		mbedtls_x509_crt_init(&ctx->tls.mbedtls.ca_cert);
#endif
	}
#endif /* MBEDTLS_X509_CRT_PARSE_C */

#if defined(CONFIG_NET_APP_SERVER)
	if (client_or_server == MBEDTLS_SSL_IS_SERVER) {
		mbedtls_pk_init(&ctx->tls.mbedtls.pkey);
	}
#endif

	mbedtls_ssl_init(&ctx->tls.mbedtls.ssl);
	mbedtls_ssl_config_init(&ctx->tls.mbedtls.conf);
	mbedtls_entropy_init(&ctx->tls.mbedtls.entropy);
	mbedtls_ctr_drbg_init(&ctx->tls.mbedtls.ctr_drbg);

#if defined(MBEDTLS_DEBUG_C) && defined(CONFIG_NET_DEBUG_APP)
	mbedtls_debug_set_threshold(DEBUG_THRESHOLD);
	mbedtls_ssl_conf_dbg(&ctx->tls.mbedtls.conf, my_debug, NULL);
#endif

	/* Seed the RNG */
	mbedtls_entropy_add_source(&ctx->tls.mbedtls.entropy,
				   ctx->tls.mbedtls.entropy_src_cb,
				   NULL,
				   MBEDTLS_ENTROPY_MAX_GATHER,
				   MBEDTLS_ENTROPY_SOURCE_STRONG);

	ret = mbedtls_ctr_drbg_seed(
		&ctx->tls.mbedtls.ctr_drbg,
		mbedtls_entropy_func,
		&ctx->tls.mbedtls.entropy,
		(const unsigned char *)ctx->tls.mbedtls.personalization_data,
		ctx->tls.mbedtls.personalization_data_len);
	if (ret != 0) {
		_net_app_print_error("mbedtls_ctr_drbg_seed returned -0x%x",
				     ret);
		goto exit;
	}

	/* Setup SSL defaults etc. */
	ret = mbedtls_ssl_config_defaults(&ctx->tls.mbedtls.conf,
					  client_or_server,
					  MBEDTLS_SSL_TRANSPORT_STREAM,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		_net_app_print_error("mbedtls_ssl_config_defaults "
				     "returned -0x%x", ret);
		goto exit;
	}

	mbedtls_ssl_conf_rng(&ctx->tls.mbedtls.conf,
			     mbedtls_ctr_drbg_random,
			     &ctx->tls.mbedtls.ctr_drbg);

	if (client_or_server == MBEDTLS_SSL_IS_SERVER) {
		/* Load the certificates and private RSA key. This needs to be
		 * done by the user so we call a callback that user must have
		 * provided.
		 */
#if defined(CONFIG_NET_APP_SERVER)
		ret = ctx->tls.mbedtls.cert_cb(ctx, &ctx->tls.mbedtls.srvcert,
					       &ctx->tls.mbedtls.pkey);
		if (ret != 0) {
			goto exit;
		}
#endif
	} else {
#if defined(CONFIG_NET_APP_CLIENT)
		ret = ctx->tls.mbedtls.ca_cert_cb(ctx,
						  &ctx->tls.mbedtls.ca_cert);
		if (ret != 0) {
			goto exit;
		}
#endif
	}

#if defined(MBEDTLS_X509_CRT_PARSE_C) && defined(CONFIG_NET_APP_SERVER)
	if (client_or_server == MBEDTLS_SSL_IS_SERVER) {
		mbedtls_ssl_conf_ca_chain(&ctx->tls.mbedtls.conf,
					  ctx->tls.mbedtls.srvcert.next,
					  NULL);

		ret = mbedtls_ssl_conf_own_cert(&ctx->tls.mbedtls.conf,
						&ctx->tls.mbedtls.srvcert,
						&ctx->tls.mbedtls.pkey);
		if (ret != 0) {
			_net_app_print_error("mbedtls_ssl_conf_own_cert "
					     "returned -0x%x", ret);
			goto exit;
		}
	}
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	ret = mbedtls_ssl_setup(&ctx->tls.mbedtls.ssl,
				&ctx->tls.mbedtls.conf);
	if (ret != 0) {
		_net_app_print_error("mbedtls_ssl_setup returned -0x%x", ret);
		goto exit;
	}

#if defined(MBEDTLS_X509_CRT_PARSE_C) && defined(CONFIG_NET_APP_CLIENT)
	if (client_or_server == MBEDTLS_SSL_IS_CLIENT &&
	    ctx->tls.cert_host) {
		ret = mbedtls_ssl_set_hostname(&ctx->tls.mbedtls.ssl,
					       ctx->tls.cert_host);
		if (ret != 0) {
			_net_app_print_error(
				"mbedtls_ssl_set_hostname returned -0x%x",
				ret);
			goto exit;
		}
	}
#endif

	NET_DBG("SSL %s setup done",
		client_or_server == MBEDTLS_SSL_IS_CLIENT ? "client" :
							    "server");

exit:
	/* The mbedtls resources are freed by _net_app_tls_handler_stop()
	 * which is called if this routine returns < 0
	 */
	return ret;
}

void _net_app_tls_handler_stop(struct net_app_ctx *ctx)
{
	mbedtls_ssl_free(&ctx->tls.mbedtls.ssl);
	mbedtls_ssl_config_free(&ctx->tls.mbedtls.conf);
	mbedtls_ctr_drbg_free(&ctx->tls.mbedtls.ctr_drbg);
	mbedtls_entropy_free(&ctx->tls.mbedtls.entropy);

	/* Empty the fifo just in case there is any received packets
	 * still there.
	 */
	while (1) {
		struct net_app_fifo_block *tx_rx_data;

		tx_rx_data = k_fifo_get(&ctx->tls.mbedtls.ssl_ctx.tx_rx_fifo,
					K_NO_WAIT);
		if (!tx_rx_data) {
			break;
		}

		net_pkt_unref(tx_rx_data->pkt);

		k_mem_pool_free(&tx_rx_data->block);
	}

	NET_DBG("TLS thread %p stopped", ctx->tls.tid);

	k_thread_abort(ctx->tls.tid);
	ctx->tls.tid = 0;
}
#endif /* CONFIG_NET_APP_TLS */

