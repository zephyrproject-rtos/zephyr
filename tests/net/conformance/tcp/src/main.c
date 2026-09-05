/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the TCP conformance suite.
 *
 * Two halves, because the suite has to look at both directions. A server that
 * accepts a connection and echoes what it is sent, so that the suite can open
 * one and drive it; and a client that connects out over and over, so that the
 * suite can look at what Zephyr sends when it is the one starting.
 *
 * Nothing here is a test hook. The suite drives it entirely from the link, the
 * way it drives the other conformance systems under test, so the TCP it
 * exercises is the one that ships rather than one built for testing.
 *
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/tcp.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tcp_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

#define ECHO_PORT	4242
#define CONNECT_PORT	4243
#define CONNECT_INTERVAL K_SECONDS(3)
#define ECHO_STACK_SIZE	2048
#define BUFFER_SIZE	256

static void echo_server(void *p1, void *p2, void *p3)
{
	struct net_sockaddr_in bind_addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(ECHO_PORT),
		.sin_addr = { .s_addr = 0 },
	};
	uint8_t buffer[BUFFER_SIZE];
	int listener;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	listener = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (listener < 0) {
		LOG_ERR("Cannot create a listening socket (%d)", -errno);
		return;
	}

	if (zsock_bind(listener, (struct net_sockaddr *)&bind_addr,
		       sizeof(bind_addr)) < 0) {
		LOG_ERR("Cannot bind to port %d (%d)", ECHO_PORT, -errno);
		return;
	}

	/* More than one, so that the stack completes a handshake even while
	 * this loop is still busy with the connection before it. A suite that
	 * opens a connection right after closing one should not have to wait
	 * for the application to come back round to accept.
	 */
	if (zsock_listen(listener, 4) < 0) {
		LOG_ERR("Cannot listen (%d)", -errno);
		return;
	}

	while (true) {
		int client = zsock_accept(listener, NULL, NULL);

		if (client < 0) {
			continue;
		}

		while (true) {
			ssize_t got = zsock_recv(client, buffer, sizeof(buffer), 0);

			if (got <= 0) {
				break;
			}

			if (zsock_send(client, buffer, got, 0) < 0) {
				break;
			}
		}

		(void)zsock_close(client);
	}
}

K_THREAD_DEFINE(echo_thread, ECHO_STACK_SIZE, echo_server, NULL, NULL, NULL,
		K_PRIO_COOP(7), 0, 0);

/* Connect out, say something, and go away again. What the suite looks at is
 * the attempt rather than what comes of it, so a refused connection is not an
 * error here.
 */
static void connect_to_peer(void)
{
	static const char payload[] = "tcp";
	struct net_sockaddr_in peer = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(CONNECT_PORT),
	};
	int sock;

	if (net_addr_pton(NET_AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
			  &peer.sin_addr) < 0) {
		LOG_ERR("Cannot parse the peer address");
		return;
	}

	sock = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (sock < 0) {
		return;
	}

	if (zsock_connect(sock, (struct net_sockaddr *)&peer, sizeof(peer)) == 0) {
		(void)zsock_send(sock, payload, sizeof(payload) - 1, 0);
	}

	(void)zsock_close(sock);
}

int main(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	LOG_INF("TCP echo server ready");

	while (true) {
		connect_to_peer();
		k_sleep(CONNECT_INTERVAL);
	}

	return 0;
}
