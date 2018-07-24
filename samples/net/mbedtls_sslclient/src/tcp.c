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

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif

#include "mbedtls/ssl.h"

int tcp_tx(void *context, const unsigned char *buf, size_t size)
{
	struct tcp_context *ctx = context;

	return send(ctx->socket, buf, size, 0);
}

int tcp_rx(void *context, unsigned char *buf, size_t size)
{
	struct tcp_context *ctx = context;

	return recv(ctx->socket, buf, size, 0);
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

	rc = socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (rc < 0) {
		printk("socket creation error\n");
		return rc;
	}

	ctx->socket = rc;
	rc = bind(ctx->socket, &ctx->local_sock, addr_len);
	if (rc) {
		printk("socket bind error\n");
		goto lb_exit;
	}

	rc = set_addr(&server_sock, server_addr, server_port);
	if (rc) {
		printk("set_addr (server) error\n");
		goto lb_exit;
	}

	rc = connect(ctx->socket, &server_sock, addr_len);
	if (rc) {
		printk("socket connect error\n");
		goto lb_exit;
	}

	return 0;

lb_exit:
	close(ctx->socket);
	return rc;
}
