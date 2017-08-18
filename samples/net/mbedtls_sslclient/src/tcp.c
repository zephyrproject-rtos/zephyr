/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <misc/printk.h>

#include "tcp_cfg.h"
#include "tcp.h"

#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_pkt.h>
#include <net/net_if.h>

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif

#include "mbedtls/ssl.h"

#define INET_FAMILY		AF_INET

#define TCP_BUF_CTR    5
#define TCP_BUF_SIZE   1024

NET_BUF_POOL_DEFINE(tcp_msg_pool, TCP_BUF_CTR, TCP_BUF_SIZE, 0, NULL);

static void tcp_received(struct net_context *context,
			 struct net_pkt *pkt, int status, void *user_data)
{
	ARG_UNUSED(context);
	struct tcp_context *ctx = user_data;

	ctx->rx_pkt = pkt;
}

int tcp_tx(void *context, const unsigned char *buf, size_t size)
{
	struct tcp_context *ctx = context;
	struct net_context *tcp_ctx;
	struct net_pkt *send_pkt;
	int rc, len;

	tcp_ctx = ctx->net_ctx;

	send_pkt = net_pkt_get_tx(tcp_ctx, K_FOREVER);
	if (!send_pkt) {
		printk("cannot create pkt\n");
		return -EIO;
	}

	len = net_pkt_append(send_pkt, size, (u8_t *) buf, K_FOREVER);

	rc = net_context_send(send_pkt, NULL, ctx->timeout, NULL, NULL);

	if (rc < 0) {
		printk("Cannot send IPv4 data to peer (%d)\n", rc);
		net_pkt_unref(send_pkt);
		return -EIO;
	} else {
		return len;
	}
}

int tcp_rx(void *context, unsigned char *buf, size_t size)
{
	struct net_buf *data = NULL;
	struct tcp_context *ctx = context;
	int rc;
	u8_t *ptr;
	int read_bytes;
	u16_t offset;

	rc = net_context_recv(ctx->net_ctx, tcp_received, K_FOREVER, ctx);
	if (rc != 0) {
		printk("net_context_recv failed with code:%d\n", rc);
		return 0;
	}
	read_bytes = net_pkt_appdatalen(ctx->rx_pkt);

	data = net_buf_alloc(&tcp_msg_pool, APP_SLEEP_MSECS);
	if (data == NULL) {
		net_pkt_unref(ctx->rx_pkt);
		printk("net_buf_alloc failed\n");
		return -EIO;
	}

	offset = net_pkt_get_len(ctx->rx_pkt) - read_bytes;
	rc = net_frag_linear_copy(data, ctx->rx_pkt->frags, offset, read_bytes);
	ptr = net_pkt_appdata(ctx->rx_pkt);
	memcpy(buf, ptr, read_bytes);

	net_pkt_unref(ctx->rx_pkt);
	net_buf_unref(data);

	return read_bytes;
}

static int set_addr(struct sockaddr *sock_addr, const char *addr,
		    u16_t server_port)
{
	void *ptr = NULL;
	int rc;

#ifdef CONFIG_NET_IPV6
	net_sin6(sock_addr)->sin6_port = htons(server_port);
	sock_addr->sa_family = AF_INET6;
	ptr = &(net_sin6(sock_addr)->sin6_addr);
	rc = net_addr_pton(AF_INET6, addr, ptr);
#else
	net_sin(sock_addr)->sin_port = htons(server_port);
	sock_addr->sa_family = AF_INET;
	ptr = &(net_sin(sock_addr)->sin_addr);
	rc = net_addr_pton(AF_INET, addr, ptr);
#endif

	if (rc) {
		printk("Invalid IP address: %s\n", addr);
	}

	return rc;
}

static int if_addr_add(struct sockaddr *local_sock)
{
	void *p = NULL;

#ifdef CONFIG_NET_IPV6
	p = net_if_ipv6_addr_add(net_if_get_default(),
				&net_sin6(local_sock)->sin6_addr,
				NET_ADDR_MANUAL, 0);
#else
	p = net_if_ipv4_addr_add(net_if_get_default(),
				&net_sin(local_sock)->sin_addr,
				NET_ADDR_MANUAL, 0);
#endif

	if (p) {
		return 0;
	}

	return -EINVAL;
}

int tcp_set_local_addr(struct tcp_context *ctx, const char *local_addr)
{
	int rc;

	rc = set_addr(&ctx->local_sock, local_addr, 0);
	if (rc) {
		printk("set_addr (local) error\n");
		goto lb_exit;
	}

	rc = if_addr_add(&ctx->local_sock);
	if (rc) {
		printk("if_addr_add error\n");
	}

lb_exit:
	return rc;
}

int tcp_init(struct tcp_context *ctx, const char *server_addr,
	     u16_t server_port)
{
	struct sockaddr server_sock;
	int rc;

#ifdef CONFIG_NET_IPV6
	socklen_t addr_len = sizeof(struct sockaddr_in6);
	sa_family_t family = AF_INET6;
#else
	socklen_t addr_len = sizeof(struct sockaddr_in);
	sa_family_t family = AF_INET;
#endif

	rc = net_context_get(family, SOCK_STREAM, IPPROTO_TCP, &ctx->net_ctx);
	if (rc) {
		printk("net_context_get error\n");
		return rc;
	}

	rc = net_context_bind(ctx->net_ctx, &ctx->local_sock, addr_len);
	if (rc) {
		printk("net_context_bind error\n");
		goto lb_exit;
	}

	rc = set_addr(&server_sock, server_addr, server_port);
	if (rc) {
		printk("set_addr (server) error\n");
		goto lb_exit;
	}

	rc = net_context_connect(ctx->net_ctx, &server_sock, addr_len, NULL,
				 ctx->timeout, NULL);
	if (rc) {
		printk("net_context_connect error\n");
		goto lb_exit;
	}

	return 0;

lb_exit:
	net_context_put(ctx->net_ctx);

	return rc;
}
