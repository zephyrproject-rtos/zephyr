/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <net/net_context.h>

#include <misc/printk.h>

#include "http_types.h"
#include "http_server.h"
#include "http_utils.h"
#include "http_write_utils.h"
#include "config.h"

/* Sets the network parameters */
static
int network_setup(struct net_context **net_ctx, net_tcp_accept_cb_t accept_cb,
		  const char *addr, uint16_t port);

#if defined(CONFIG_MBEDTLS)
#include "ssl_utils.h"
#endif

void main(void)
{
	struct net_context *net_ctx = NULL;

	http_ctx_init();

	http_url_default_handler(http_write_soft_404_not_found);
	http_url_add("/headers", HTTP_URL_STANDARD, http_write_header_fields);
	http_url_add("/index.html", HTTP_URL_STANDARD, http_write_it_works);

	network_setup(&net_ctx, http_accept_cb, ZEPHYR_ADDR, ZEPHYR_PORT);
}

static
int network_setup(struct net_context **net_ctx, net_tcp_accept_cb_t accept_cb,
		  const char *addr, uint16_t port)
{
	struct sockaddr local_sock;
	void *ptr;
	int rc;

	*net_ctx = NULL;

#ifdef CONFIG_NET_IPV6
	net_sin6(&local_sock)->sin6_port = htons(port);
	local_sock.family = AF_INET6;
	ptr = &(net_sin6(&local_sock)->sin6_addr);
	rc = net_addr_pton(AF_INET6, addr, ptr);
#else
	net_sin(&local_sock)->sin_port = htons(port);
	local_sock.family = AF_INET;
	ptr = &(net_sin(&local_sock)->sin_addr);
	rc = net_addr_pton(AF_INET, addr, ptr);
#endif

	if (rc) {
		printk("Invalid IP address/Port: %s, %d\n", addr, port);
		return rc;
	}

#ifdef CONFIG_NET_IPV6
	ptr = net_if_ipv6_addr_add(net_if_get_default(),
				   &net_sin6(&local_sock)->sin6_addr,
				   NET_ADDR_MANUAL, 0);
#else
	ptr = net_if_ipv4_addr_add(net_if_get_default(),
				   &net_sin(&local_sock)->sin_addr,
				   NET_ADDR_MANUAL, 0);
#endif

#if defined(CONFIG_NET_IPV6)
	rc = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, net_ctx);
#else
	rc = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, net_ctx);
#endif
	if (rc != 0) {
		printk("net_context_get error\n");
		return rc;
	}

	rc = net_context_bind(*net_ctx, (const struct sockaddr *)&local_sock,
			      sizeof(local_sock));
	if (rc != 0) {
		printk("net_context_bind error\n");
		goto lb_error;
	}

	rc = net_context_listen(*net_ctx, 0);
	if (rc != 0) {
		printk("net_context_listen error\n");
		goto lb_error;
	}

	rc = net_context_accept(*net_ctx, accept_cb, 0, NULL);
	if (rc != 0) {
		printk("net_context_accept error\n");
		goto lb_error;
	}

	print_server_banner(&local_sock);

#if defined(CONFIG_MBEDTLS)
	https_server_start();
#endif
	return 0;

lb_error:
	net_context_put(*net_ctx);
	*net_ctx = NULL;

	return rc;
}
