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
#include <net/nbuf.h>
#include <net/net_if.h>

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif

#include "mbedtls/ssl.h"

#define CLIENT_IP_ADDR		{ { { CLIENT_IPADDR0, CLIENT_IPADDR1,	\
				      CLIENT_IPADDR2, CLIENT_IPADDR3 } } }

#define SERVER_IP_ADDR		{ { { SERVER_IPADDR0, SERVER_IPADDR1,	\
				      SERVER_IPADDR2, SERVER_IPADDR3 } } }

#define INET_FAMILY		AF_INET

static struct in_addr client_addr = CLIENT_IP_ADDR;
static struct in_addr server_addr = SERVER_IP_ADDR;

static void tcp_received(struct net_context *context,
			 struct net_buf *buf, int status, void *user_data)
{
	ARG_UNUSED(context);
	struct tcp_context *ctx = user_data;

	ctx->rx_nbuf = buf;
}

int tcp_tx(void *context, const unsigned char *buf, size_t size)
{
	struct tcp_context *ctx = context;
	struct net_context *tcp_ctx;
	struct net_buf *send_buf;
	struct sockaddr_in dst_addr;
	int rc, len;

	tcp_ctx = ctx->net_ctx;

	send_buf = net_nbuf_get_tx(tcp_ctx, K_FOREVER);
	if (!send_buf) {
		printk("cannot create buf\n");
		return -EIO;
	}

	rc = net_nbuf_append(send_buf, size, (uint8_t *) buf, K_FOREVER);
	if (!rc) {
		printk("cannot write buf\n");
		return -EIO;
	}

	net_ipaddr_copy(&dst_addr.sin_addr, &server_addr);
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = htons(SERVER_PORT);

	len = net_buf_frags_len(send_buf);
	k_sleep(TCP_TX_TIMEOUT);

	rc = net_context_sendto(send_buf, (struct sockaddr *)&dst_addr,
				sizeof(struct sockaddr_in),
				NULL, K_FOREVER, NULL, NULL);

	if (rc < 0) {
		printk("Cannot send IPv4 data to peer (%d)\n", rc);
		net_nbuf_unref(send_buf);
		return -EIO;
	} else {
		return len;
	}
}

int tcp_rx(void *context, unsigned char *buf, size_t size)
{
	struct tcp_context *ctx = context;
	uint8_t *ptr;
	int pos = 0;
	int len;
	struct net_buf *rx_buf;
	int rc, read_bytes;

	rc = net_context_recv(ctx->net_ctx, tcp_received, K_FOREVER, ctx);
	if (rc != 0) {
		return 0;
	}
	read_bytes = net_nbuf_appdatalen(ctx->rx_nbuf);

	ptr = net_nbuf_appdata(ctx->rx_nbuf);
	rx_buf = ctx->rx_nbuf->frags;
	len = rx_buf->len - (ptr - rx_buf->data);

	while (rx_buf) {
		memcpy(buf + pos, ptr, len);
		pos += len;
		rx_buf = rx_buf->frags;
		if (!rx_buf) {
			break;
		}
		ptr = rx_buf->data;
		len = rx_buf->len;
	}

	if (read_bytes != pos) {
		return -EIO;
	}
	rc = read_bytes;
	net_nbuf_unref(ctx->rx_nbuf);
	ctx->remaining = 0;
	ctx->rx_nbuf = NULL;
	return rc;
}

int tcp_init(struct tcp_context *ctx)
{
	struct net_context *tcp_ctx = { 0 };
	struct sockaddr_in my_addr4 = { 0 };
	int rc;

	net_ipaddr_copy(&my_addr4.sin_addr, &client_addr);
	my_addr4.sin_family = AF_INET;
	my_addr4.sin_port = htons(CLIENT_PORT);

	rc = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_TCP, &tcp_ctx);
	if (rc < 0) {
		printk("Cannot get network context for IPv4 TCP (%d)", rc);
		return -EIO;
	}

	rc = net_context_bind(tcp_ctx, (struct sockaddr *)&my_addr4,
			      sizeof(struct sockaddr_in));
	if (rc < 0) {
		printk("Cannot bind IPv4 TCP port %d (%d)", CLIENT_PORT, rc);
		goto error;
	}

	ctx->rx_nbuf = NULL;
	ctx->remaining = 0;
	ctx->net_ctx = tcp_ctx;

	return 0;

error:
	net_context_put(tcp_ctx);
	return -EINVAL;
}
