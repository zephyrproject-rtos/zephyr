/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif

#include "mbedtls/ssl.h"

#include "config.h"
#include "ssl_utils.h"

#define RX_FIFO_DEPTH 4

K_MEM_POOL_DEFINE(rx_pkts, 4, 64, RX_FIFO_DEPTH, 4);

static void ssl_received(struct net_context *context,
			 struct net_buf *buf, int status, void *user_data)
{
	struct ssl_context *ctx = user_data;
	struct rx_fifo_block *rx_data = NULL;
	struct k_mem_block block;

	ARG_UNUSED(context);
	ARG_UNUSED(status);

	if (!net_nbuf_appdatalen(buf)) {
		net_nbuf_unref(buf);
		return;
	}

	k_mem_pool_alloc(&rx_pkts, &block,
			 sizeof(struct rx_fifo_block), K_FOREVER);
	rx_data = block.data;
	rx_data->buf = buf;

	/* For freeing memory later */
	memcpy(&rx_data->block, &block, sizeof(struct k_mem_block));
	k_fifo_put(&ctx->rx_fifo, (void *)rx_data);
}

static inline void ssl_sent(struct net_context *context,
			    int status, void *token, void *user_data)
{
	struct ssl_context *ctx = user_data;

	k_sem_give(&ctx->tx_sem);
}

int ssl_tx(void *context, const unsigned char *buf, size_t size)
{
	struct ssl_context *ctx = context;
	struct net_context *net_ctx;
	struct net_buf *send_buf;

	int rc, len;

	net_ctx = ctx->net_ctx;

	send_buf = net_nbuf_get_tx(net_ctx, K_NO_WAIT);
	if (!send_buf) {
		return MBEDTLS_ERR_SSL_ALLOC_FAILED;
	}

	rc = net_nbuf_append(send_buf, size, (uint8_t *) buf, K_FOREVER);
	if (!rc) {
		net_nbuf_unref(send_buf);
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}

	len = net_buf_frags_len(send_buf);

	rc = net_context_send(send_buf, ssl_sent, K_NO_WAIT, NULL, ctx);

	if (rc < 0) {
		net_nbuf_unref(send_buf);
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}

	k_sem_take(&ctx->tx_sem, K_FOREVER);
	return len;
}

int ssl_rx(void *context, unsigned char *buf, size_t size)
{
	struct ssl_context *ctx = context;
	uint16_t read_bytes;
	struct rx_fifo_block *rx_data;
	uint8_t *ptr;
	int pos;
	int len;
	int rc = 0;

	if (ctx->frag == NULL) {
		rx_data = k_fifo_get(&ctx->rx_fifo, K_FOREVER);
		ctx->rx_nbuf = rx_data->buf;
		k_mem_pool_free(&rx_data->block);

		read_bytes = net_nbuf_appdatalen(ctx->rx_nbuf);

		ctx->remaining = read_bytes;
		ctx->frag = ctx->rx_nbuf->frags;
		ptr = net_nbuf_appdata(ctx->rx_nbuf);

		len = ptr - ctx->frag->data;
		net_buf_pull(ctx->frag, len);
	} else {
		read_bytes = ctx->remaining;
		ptr = ctx->frag->data;
	}

	len = ctx->frag->len;
	pos = 0;
	if (read_bytes > size) {
		while (ctx->frag) {
			read_bytes = len < (size - pos) ? len : (size - pos);
			memcpy(buf + pos, ptr, read_bytes);
			pos += read_bytes;
			if (pos < size) {
				ctx->frag = ctx->frag->frags;
				ptr = ctx->frag->data;
				len = ctx->frag->len;
			} else {
				if (read_bytes == len) {
					ctx->frag = ctx->frag->frags;
				} else {
					net_buf_pull(ctx->frag, read_bytes);
				}

				ctx->remaining -= size;
				return size;
			}
		}
	} else {
		while (ctx->frag) {
			memcpy(buf + pos, ptr, len);
			pos += len;
			ctx->frag = ctx->frag->frags;
			if (!ctx->frag) {
				break;
			}

			ptr = ctx->frag->data;
			len = ctx->frag->len;
		}

		net_nbuf_unref(ctx->rx_nbuf);
		ctx->rx_nbuf = NULL;
		ctx->frag = NULL;
		ctx->remaining = 0;

		if (read_bytes != pos) {
			return -EIO;
		}

		rc = read_bytes;
	}

	return rc;
}

static void ssl_accepted(struct net_context *context,
			 struct sockaddr *addr,
			 socklen_t addrlen, int error, void *user_data)
{
	int ret;
	struct ssl_context *ctx = user_data;

	ctx->net_ctx = context;
	ret = net_context_recv(context, ssl_received, 0, user_data);
	if (ret < 0) {
		printk("Cannot receive TCP packet (family %d)",
		       net_context_get_family(context));
	}

}

#if defined(CONFIG_NET_IPV6)
int ssl_init(struct ssl_context *ctx, void *addr)
{
	struct net_context *tcp_ctx = { 0 };
	struct sockaddr_in6 my_addr = { 0 };
	struct in6_addr *server_addr = addr;
	int rc;

	k_sem_init(&ctx->tx_sem, 0, UINT_MAX);
	k_fifo_init(&ctx->rx_fifo);

	my_mcast_addr.sin6_family = AF_INET6;

	net_ipaddr_copy(&my_addr.sin6_addr, server_addr);
	my_addr.sin6_family = AF_INET6;
	my_addr.sin6_port = htons(SERVER_PORT);

	rc = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, &tcp_ctx);
	if (rc < 0) {
		printk("Cannot get network context for IPv6 TCP (%d)", rc);
		return -EIO;
	}

	rc = net_context_bind(tcp_ctx, (struct sockaddr *)&my_addr,
			      sizeof(struct sockaddr_in6));
	if (rc < 0) {
		printk("Cannot bind IPv6 TCP port %d (%d)", SERVER_PORT, rc);
		goto error;
	}

	ctx->rx_nbuf = NULL;
	ctx->remaining = 0;
	ctx->net_ctx = tcp_ctx;

	rc = net_context_listen(ctx->net_ctx, 0);
	if (rc < 0) {
		printk("Cannot listen IPv6 TCP (%d)", rc);
		return -EIO;
	}

	rc = net_context_accept(ctx->net_ctx, ssl_accepted, 0, ctx);
	if (rc < 0) {
		printk("Cannot accept IPv4 (%d)", rc);
		return -EIO;
	}

	return 0;

error:
	net_context_put(tcp_ctx);
	return -EINVAL;
}

#else
int ssl_init(struct ssl_context *ctx, void *addr)
{
	struct net_context *tcp_ctx = { 0 };
	struct sockaddr_in my_addr4 = { 0 };
	struct in_addr *server_addr = addr;
	int rc;

	k_sem_init(&ctx->tx_sem, 0, UINT_MAX);
	k_fifo_init(&ctx->rx_fifo);

	net_ipaddr_copy(&my_addr4.sin_addr, server_addr);
	my_addr4.sin_family = AF_INET;
	my_addr4.sin_port = htons(SERVER_PORT);

	rc = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &tcp_ctx);
	if (rc < 0) {
		printk("Cannot get network context for IPv4 TCP (%d)", rc);
		return -EIO;
	}

	rc = net_context_bind(tcp_ctx, (struct sockaddr *)&my_addr4,
			      sizeof(struct sockaddr_in));
	if (rc < 0) {
		printk("Cannot bind IPv4 TCP port %d (%d)", SERVER_PORT, rc);
		goto error;
	}

	ctx->rx_nbuf = NULL;
	ctx->remaining = 0;
	ctx->net_ctx = tcp_ctx;

	rc = net_context_listen(ctx->net_ctx, 0);
	if (rc < 0) {
		printk("Cannot listen IPv4 (%d)", rc);
		return -EIO;
	}

	rc = net_context_accept(ctx->net_ctx, ssl_accepted, 0, ctx);
	if (rc < 0) {
		printk("Cannot accept IPv4 (%d)", rc);
		return -EIO;
	}

	return 0;

error:
	net_context_put(tcp_ctx);
	return -EINVAL;
}
#endif
