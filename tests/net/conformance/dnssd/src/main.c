/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the DNS service discovery conformance suite.
 *
 * The application registers one service and waits. Everything the suite looks
 * at is the responder answering, so there is nothing to drive from here.
 *
 * The socket matters even though nothing ever connects to it. The responder
 * checks that a service is bound before advertising it, so a registration
 * whose port nothing listens on is silently never mentioned. Listening is what
 * makes the service real enough to be advertised.
 *
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/dnssd.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dnssd_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/dns_sd.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>

#define SERVICE_PORT	4242

DNS_SD_REGISTER_TCP_SERVICE(conformance_service, CONFIG_NET_HOSTNAME, "_zephyr",
			    "local", DNS_SD_EMPTY_TXT, SERVICE_PORT);

/* Bind and listen, so that the responder sees the service as bound. Nothing
 * connects, so there is no accept loop.
 */
static int listen_for_the_service(void)
{
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = NET_IN6ADDR_ANY_INIT,
		.sin6_port = net_htons(SERVICE_PORT),
	};
	int sock;

	sock = zsock_socket(NET_AF_INET6, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (sock < 0) {
		return -errno;
	}

	if (zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr)) < 0) {
		(void)zsock_close(sock);
		return -errno;
	}

	if (zsock_listen(sock, 1) < 0) {
		(void)zsock_close(sock);
		return -errno;
	}

	return 0;
}

int main(void)
{
	struct net_if *iface = net_if_get_default();
	int ret;

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	/* The addresses are configured during initialisation, so the interface
	 * is usually up before main() runs. Only wait when it is not: waiting
	 * unconditionally would block until the timeout, because the event has
	 * already been and gone.
	 */
	if (!net_if_is_up(iface)) {
		(void)net_mgmt_event_wait_on_iface(iface, NET_EVENT_IF_UP, NULL,
						   NULL, NULL, K_SECONDS(10));
	}

	ret = listen_for_the_service();
	if (ret < 0) {
		LOG_ERR("Cannot listen on the advertised port (%d)", ret);
		return ret;
	}

	LOG_INF("Service discovery ready");

	k_sleep(K_FOREVER);

	return 0;
}
