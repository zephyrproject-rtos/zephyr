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

static void network_setup(void);

void main(void)
{
	http_ctx_init();

	http_url_default_handler(http_write_soft_404_not_found);
	http_url_add("/headers", HTTP_URL_STANDARD, http_write_header_fields);
	http_url_add("/index.html", HTTP_URL_STANDARD, http_write_it_works);

	network_setup();
}

/**
 * @brief connection_cb		Callback executed when a connection is accepted
 * @param ctx			Network context
 * @param addr			Client's address
 * @param addr_len		Address length
 * @param status		Status code, 0 on success, < 0 otherwise
 * @param data			User-provided data
 */
void connection_cb(struct net_context *net_ctx, struct sockaddr *addr,
		   socklen_t addr_len, int status, void *data)
{
	struct http_server_ctx *http_ctx = NULL;

	ARG_UNUSED(addr_len);
	ARG_UNUSED(data);

	if (status != 0) {
		printk("Status code: %d\n", status);
		net_context_put(net_ctx);
		return;
	}

	print_client_banner(addr);

	http_ctx = http_ctx_get();
	if (!http_ctx) {
		printk("Unable to get free HTTP context\n");
		net_context_put(net_ctx);
		return;
	}

	http_ctx_set(http_ctx, net_ctx);

	net_context_recv(net_ctx, http_rx_tx, 0, http_ctx);
}

/**
 * @brief network_setup		This routine configures the HTTP server.
 *				It puts the application in listening mode and
 *				installs the accept connection callback.
 */
static void network_setup(void)
{
	struct net_context *ctx = NULL;
	struct sockaddr local_sock;
	void *ptr;
	int rc;

#ifdef CONFIG_NET_IPV6
	net_sin6(&local_sock)->sin6_port = htons(ZEPHYR_PORT);
	sock_addr.family = AF_INET6;
	ptr = &(net_sin6(&local_sock)->sin6_addr);
	rc = net_addr_pton(AF_INET6, ZEPHYR_ADDR, ptr);
#else
	net_sin(&local_sock)->sin_port = htons(ZEPHYR_PORT);
	local_sock.family = AF_INET;
	ptr = &(net_sin(&local_sock)->sin_addr);
	rc = net_addr_pton(AF_INET, ZEPHYR_ADDR, ptr);
#endif

	if (rc) {
		printk("Invalid IP address/Port: %s, %d\n",
		       ZEPHYR_ADDR, ZEPHYR_PORT);
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
	print_server_banner(&local_sock);

#if defined(CONFIG_NET_IPV6)
	rc = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, &ctx);
#else
	rc = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
#endif
	if (rc != 0) {
		printk("net_context_get error\n");
		return;
	}

	rc = net_context_bind(ctx, (const struct sockaddr *)&local_sock,
			      sizeof(local_sock));
	if (rc != 0) {
		printk("net_context_bind error\n");
		goto lb_error;
	}

	rc = net_context_listen(ctx, 0);
	if (rc != 0) {
		printk("[%s:%d] net_context_listen %d, <%s>\n",
		       __func__, __LINE__, rc, RC_STR(rc));
		goto lb_error;
	}

	rc = net_context_accept(ctx, connection_cb, 0, NULL);
	if (rc != 0) {
		printk("[%s:%d] net_context_accept %d, <%s>\n",
		       __func__, __LINE__, rc, RC_STR(rc));
		goto lb_error;
	}

	return;

lb_error:
	net_context_put(ctx);
}
