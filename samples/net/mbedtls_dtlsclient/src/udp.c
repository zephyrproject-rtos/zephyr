/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>

#include "udp_cfg.h"
#include "udp.h"

#if defined(CONFIG_NET_IPV6)
static struct in6_addr server_addr;
static struct in6_addr mcast_addr = MCAST_IP_ADDR;
static const socklen_t addrlen = sizeof(struct sockaddr_in6);

static void set_destination(struct sockaddr *addr)
{
	struct sockaddr_in6 *dst_addr = (struct sockaddr_in6 *)addr;

	net_ipaddr_copy(&dst_addr->sin6_addr, &server_addr);
	dst_addr->sin6_family = AF_INET6;
	dst_addr->sin6_port = htons(SERVER_PORT);
}

#else
static struct in_addr server_addr;
static const socklen_t addrlen = sizeof(struct sockaddr_in);

static void set_destination(struct sockaddr *addr)
{
	struct sockaddr_in *dst_addr = (struct sockaddr_in *)addr;

	net_ipaddr_copy(&dst_addr->sin_addr, &server_addr);
	dst_addr->sin_family = AF_INET;
	dst_addr->sin_port = htons(SERVER_PORT);
}
#endif

static void udp_received(struct net_context *context,
			 struct net_pkt *pkt, int status, void *user_data)
{
	struct udp_context *ctx = user_data;

	ARG_UNUSED(context);
	ARG_UNUSED(status);

	if (ctx->rx_pkt) {
		k_sem_give(&ctx->rx_sem);
		k_yield();

		if (ctx->rx_pkt) {
			printk("Packet %p is still being handled, "
			       "dropping %p\n", ctx->rx_pkt, pkt);

			net_pkt_unref(pkt);
			return;
		}
	}

	ctx->rx_pkt = pkt;
	k_sem_give(&ctx->rx_sem);
	k_yield();
}

int udp_tx(void *context, const unsigned char *buf, size_t size)
{
	struct udp_context *ctx = context;
	struct net_context *udp_ctx;
	struct net_pkt *send_pkt;
	struct sockaddr dst_addr;
	int rc, len;

	udp_ctx = ctx->net_ctx;

	send_pkt = net_pkt_get_tx(udp_ctx, K_FOREVER);
	if (!send_pkt) {
		printk("cannot create buf\n");
		return -EIO;
	}

	rc = net_pkt_append_all(send_pkt, size, (u8_t *) buf, K_FOREVER);
	if (!rc) {
		printk("cannot write buf\n");
		net_pkt_unref(send_pkt);
		return -EIO;
	}

	set_destination(&dst_addr);
	len = net_pkt_get_len(send_pkt);
	k_sleep(UDP_TX_TIMEOUT);

	rc = net_context_sendto(send_pkt, &dst_addr,
				addrlen, NULL, K_FOREVER, NULL, NULL);
	if (rc < 0) {
		printk("Cannot send data to peer (%d)\n", rc);
		net_pkt_unref(send_pkt);
		return -EIO;
	} else {
		return len;
	}
}

int udp_rx(void *context, unsigned char *buf, size_t size)
{
	struct udp_context *ctx = context;
	struct net_buf *rx_buf = NULL;
	u16_t read_bytes;
	u8_t *ptr;
	int pos;
	int len;
	int rc;

	k_sem_take(&ctx->rx_sem, K_FOREVER);

	read_bytes = net_pkt_appdatalen(ctx->rx_pkt);
	if (read_bytes > size) {
		net_pkt_unref(ctx->rx_pkt);
		ctx->rx_pkt = NULL;
		return -ENOMEM;
	}

	ptr = net_pkt_appdata(ctx->rx_pkt);

	rx_buf = ctx->rx_pkt->frags;
	if (!rx_buf) {
		net_pkt_unref(ctx->rx_pkt);
		ctx->rx_pkt = NULL;
		return -ENOMEM;
	}

	len = rx_buf->len - (ptr - rx_buf->data);
	pos = 0;

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

	net_pkt_unref(ctx->rx_pkt);
	ctx->rx_pkt = NULL;

	if (read_bytes != pos) {
		return -EIO;
	}

	rc = read_bytes;
	ctx->remaining = 0;

	return rc;
}

#if defined(CONFIG_NET_IPV6)
int udp_init(struct udp_context *ctx)
{
	struct net_context *udp_ctx = { 0 };
	struct net_context *mcast_ctx = { 0 };
	struct sockaddr_in6 my_addr = { 0 };
	struct sockaddr_in6 my_mcast_addr = { 0 };
	int rc;

	k_sem_init(&ctx->rx_sem, 0, UINT_MAX);

	net_ipaddr_copy(&my_mcast_addr.sin6_addr, &mcast_addr);
	my_mcast_addr.sin6_family = AF_INET6;

	net_ipaddr_copy(&my_addr.sin6_addr, &client_addr);
	my_addr.sin6_family = AF_INET6;
	my_addr.sin6_port = htons(CLIENT_PORT);

	rc = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &udp_ctx);
	if (rc < 0) {
		printk("Cannot get network context for IPv6 UDP (%d)", rc);
		return -EIO;
	}

	rc = net_context_bind(udp_ctx, (struct sockaddr *)&my_addr,
			      sizeof(struct sockaddr_in6));
	if (rc < 0) {
		printk("Cannot bind IPv6 UDP port %d (%d)", CLIENT_PORT, rc);
		goto error;
	}

	rc = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &mcast_ctx);
	if (rc < 0) {
		printk("Cannot get receiving IPv6 mcast (%d)", rc);
		goto error;
	}

	rc = net_context_bind(mcast_ctx, (struct sockaddr *)&my_mcast_addr,
			      sizeof(struct sockaddr_in6));
	if (rc < 0) {
		printk("Cannot get bind IPv6 mcast (%d)", rc);
		goto error;
	}

	ctx->rx_pkt = NULL;
	ctx->remaining = 0;
	ctx->net_ctx = udp_ctx;

#if defined(CONFIG_NET_APP_PEER_IPV6_ADDR)
	if (net_addr_pton(AF_INET6,
			  CONFIG_NET_APP_PEER_IPV6_ADDR,
			  &server_addr) < 0) {
		printk("Invalid peer IPv6 address %s",
		       CONFIG_NET_APP_PEER_IPV6_ADDR);
	}
#endif

	rc = net_context_recv(ctx->net_ctx, udp_received, K_NO_WAIT, ctx);
	if (rc != 0) {
		return -EIO;
	}

	return 0;

error:
	net_context_put(udp_ctx);
	return -EINVAL;
}

#else
int udp_init(struct udp_context *ctx)
{
	struct net_context *udp_ctx = { 0 };
	struct sockaddr_in my_addr4 = { 0 };
	int rc;

	net_ipaddr_copy(&my_addr4.sin_addr, &client_addr);
	my_addr4.sin_family = AF_INET;
	my_addr4.sin_port = htons(CLIENT_PORT);

	k_sem_init(&ctx->rx_sem, 0, UINT_MAX);

	rc = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &udp_ctx);
	if (rc < 0) {
		printk("Cannot get network context for IPv4 UDP (%d)", rc);
		return -EIO;
	}

	rc = net_context_bind(udp_ctx, (struct sockaddr *)&my_addr4,
			      sizeof(struct sockaddr_in));
	if (rc < 0) {
		printk("Cannot bind IPv4 UDP port %d (%d)", CLIENT_PORT, rc);
		goto error;
	}

	ctx->rx_pkt = NULL;
	ctx->remaining = 0;
	ctx->net_ctx = udp_ctx;

#if defined(CONFIG_NET_APP_PEER_IPV4_ADDR)
	if (net_addr_pton(AF_INET, CONFIG_NET_APP_PEER_IPV4_ADDR,
			  &server_addr) < 0) {
		printk("Invalid IPv4 address %s",
		       CONFIG_NET_APP_PEER_IPV4_ADDR);
	}
#endif

	rc = net_context_recv(ctx->net_ctx, udp_received, K_NO_WAIT, ctx);
	if (rc != 0) {
		return -EIO;
	}

	return 0;

error:
	net_context_put(udp_ctx);
	return -EINVAL;
}
#endif
