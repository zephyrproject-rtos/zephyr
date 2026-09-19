/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the IPv6 neighbour discovery conformance suite.
 *
 * The application answers for its own addresses, which is what most of the
 * suite looks at, and sends a datagram to a peer every couple of seconds so
 * that there is something for it to resolve. Nothing answers on the link
 * unless the suite chooses to, so an unresolved destination keeps producing
 * solicitations for the suite to look at, the same way the address resolution
 * test does one layer down.
 *
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/ndp.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ndp_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

#define PEER_PORT	9999
#define SEND_INTERVAL	K_SECONDS(2)

static void send_to_peer(void)
{
	static const char payload[] = "ndp";
	struct net_sockaddr_in6 peer = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(PEER_PORT),
	};
	int sock;

	if (net_addr_pton(NET_AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
			  &peer.sin6_addr) < 0) {
		LOG_ERR("Cannot parse the peer address");
		return;
	}

	sock = zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
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

	LOG_INF("Neighbour discovery ready");

	while (true) {
		send_to_peer();
		k_sleep(SEND_INTERVAL);
	}

	return 0;
}
