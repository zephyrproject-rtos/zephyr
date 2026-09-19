/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the ARP conformance suite.
 *
 * Two things are needed of it. It has to answer address resolution for its own
 * address, which it does without any help from the application. And it has to
 * ask for somebody else's, which needs a reason: so it sends a datagram to a
 * peer on the link over and over, and each one that finds no cached entry puts
 * a request on the wire.
 *
 * This one runs on its own interface, with no address on the Linux side of it,
 * so that nothing but the suite answers. See net-tools/zeth-l2.conf.
 *
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/arp.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(arp_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

#define PEER_PORT	9999
#define SEND_INTERVAL	K_SECONDS(2)

static void send_to_peer(void)
{
	static const char payload[] = "arp";
	struct net_sockaddr_in peer = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(PEER_PORT),
	};
	int sock;

	if (net_addr_pton(NET_AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
			  &peer.sin_addr) < 0) {
		LOG_ERR("Cannot parse the peer address");
		return;
	}

	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Cannot create a socket (%d)", -errno);
		return;
	}

	(void)zsock_sendto(sock, payload, sizeof(payload) - 1, 0,
			   (struct net_sockaddr *)&peer, sizeof(peer));

	(void)zsock_close(sock);
}

int main(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	LOG_INF("ARP responder ready");

	while (true) {
		send_to_peer();
		k_sleep(SEND_INTERVAL);
	}

	return 0;
}
