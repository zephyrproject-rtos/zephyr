/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/printk.h>

/* Host side of the tap created by the net-tools sidecar. The sidecar assigns a
 * per-instance subnet and the runner bakes the matching peer address in.
 */
#define PEER_ADDR CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#define ECHO_PORT 4242

static K_SEM_DEFINE(reply_sem, 0, 1);

static enum net_verdict on_echo_reply(struct net_icmp_ctx *ctx, struct net_pkt *pkt,
				      struct net_icmp_ip_hdr *ip_hdr,
				      struct net_icmp_hdr *icmp_hdr, void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(pkt);
	ARG_UNUSED(ip_hdr);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	k_sem_give(&reply_sem);

	return NET_OK;
}

/* ICMP ping the host: proves the tap the sidecar created carries traffic. */
static void ping_host(struct net_if *iface)
{
	struct net_sockaddr_in dst = { .sin_family = NET_AF_INET };
	struct net_icmp_ctx ctx;

	(void)net_addr_pton(NET_AF_INET, PEER_ADDR, &dst.sin_addr);

	if (net_icmp_init_ctx(&ctx, NET_AF_INET, NET_ICMPV4_ECHO_REPLY, 0, on_echo_reply) != 0) {
		return;
	}

	printk("NET_TOOLS_SIDECAR: pinging host %s\n", PEER_ADDR);
	if (net_icmp_send_echo_request(&ctx, iface, (struct net_sockaddr *)&dst, NULL, NULL) == 0 &&
	    k_sem_take(&reply_sem, K_SECONDS(5)) == 0) {
		printk("NET_TOOLS_SIDECAR: PING REPLY OK\n");
	} else {
		printk("NET_TOOLS_SIDECAR: NO REPLY\n");
	}

	(void)net_icmp_cleanup_ctx(&ctx);
}

/* UDP echo off the companion echo-server the sidecar started on the host. */
static void echo_host(void)
{
	struct net_sockaddr_in peer = { .sin_family = NET_AF_INET,
					.sin_port = net_htons(ECHO_PORT) };
	static const char msg[] = "zephyr-net-tools-sidecar";
	char buf[sizeof(msg)];
	struct zsock_pollfd fds;
	int sock;

	(void)net_addr_pton(NET_AF_INET, PEER_ADDR, &peer.sin_addr);

	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	if (sock < 0) {
		printk("NET_TOOLS_SIDECAR: socket failed\n");
		return;
	}

	printk("NET_TOOLS_SIDECAR: echoing off %s:%d\n", PEER_ADDR, ECHO_PORT);
	(void)zsock_sendto(sock, msg, sizeof(msg), 0,
			   (struct net_sockaddr *)&peer, sizeof(peer));

	fds.fd = sock;
	fds.events = ZSOCK_POLLIN;
	if (zsock_poll(&fds, 1, 3000) > 0 &&
	    zsock_recv(sock, buf, sizeof(buf), 0) == (int)sizeof(msg) &&
	    memcmp(buf, msg, sizeof(msg)) == 0) {
		printk("NET_TOOLS_SIDECAR: UDP ECHO OK\n");
	} else {
		printk("NET_TOOLS_SIDECAR: NO ECHO\n");
	}

	(void)zsock_close(sock);
}

int main(void)
{
	struct net_if *iface = net_if_get_default();

	/* The static addresses come from the net config; wait for the link over
	 * the host tap to come up.
	 */
	for (int i = 0; (i < 50) && !net_if_is_up(iface); i++) {
		k_sleep(K_MSEC(100));
	}

	ping_host(iface);
	echo_host();

	return 0;
}
