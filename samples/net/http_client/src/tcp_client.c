/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tcp_client.h"
#include "config.h"

#include <net/net_core.h>
#include <net/net_if.h>
#include <net/nbuf.h>

#include <misc/printk.h>

static
int set_addr(struct sockaddr *sock_addr, const char *addr, uint16_t server_port)
{
	void *ptr = NULL;
	int rc;

#ifdef CONFIG_NET_IPV6
	net_sin6(sock_addr)->sin6_port = htons(server_port);
	sock_addr->family = AF_INET6;
	ptr = &(net_sin6(sock_addr)->sin6_addr);
	rc = net_addr_pton(AF_INET6, addr, ptr);
#else
	net_sin(sock_addr)->sin_port = htons(server_port);
	sock_addr->family = AF_INET;
	ptr = &(net_sin(sock_addr)->sin_addr);
	rc = net_addr_pton(AF_INET, addr, ptr);
#endif
	if (rc) {
		printk("Invalid IP address: %s\n", addr);
	}

	return rc;
}

static
int if_addr_add(struct sockaddr *local_sock)
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

int tcp_set_local_addr(struct tcp_client_ctx *ctx, const char *local_addr)
{
	int  rc;

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

static
void recv_cb(struct net_context *net_ctx, struct net_buf *rx, int status,
	     void *data)
{
	struct tcp_client_ctx *ctx = (struct tcp_client_ctx *)data;

	ARG_UNUSED(net_ctx);

	if (status) {
		return;
	}

	if (rx == NULL || net_nbuf_appdatalen(rx) == 0) {
		goto lb_exit;
	}

	/* receive_cb must take ownership of the rx buffer */
	if (ctx->receive_cb) {
		ctx->receive_cb(ctx, rx);
		return;
	}

lb_exit:
	net_buf_unref(rx);
}

int tcp_connect(struct tcp_client_ctx *ctx, const char *server_addr,
		uint16_t server_port)
{
#if CONFIG_NET_IPV6
	socklen_t addr_len = sizeof(struct sockaddr_in6);
	sa_family_t family = AF_INET6;
#else
	socklen_t addr_len = sizeof(struct sockaddr_in);
	sa_family_t family = AF_INET;
#endif
	struct sockaddr server_sock;
	int rc;

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

	(void)net_context_recv(ctx->net_ctx, recv_cb, K_NO_WAIT, ctx);

	return 0;

lb_exit:
	net_context_put(ctx->net_ctx);

	return rc;
}

int tcp_disconnect(struct tcp_client_ctx *ctx)
{
	if (ctx->net_ctx) {
		net_context_put(ctx->net_ctx);
		ctx->net_ctx = NULL;
	}

	return 0;
}
