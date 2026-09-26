/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/dns_sd.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mdns_offload_test, LOG_LEVEL_INF);

#define SERVICE_PORT 4242

/* Advertise a DNS-SD service so the host can resolve the responder's
 * hostname (and thus its A/AAAA records) through the offloaded sockets.
 */
DNS_SD_REGISTER_TCP_SERVICE(mdns_offload, CONFIG_NET_HOSTNAME, "_zephyr", "local", DNS_SD_EMPTY_TXT,
			    SERVICE_PORT);

/* Open a listening TCP socket bound to the service port so DNS-SD advertises
 * the service: the responder only answers the browse once the port is actually
 * in use. With offloaded sockets the binding lives in the offload engine, and
 * the socket layer records it for net_context_port_in_use().
 */
static int listen_on_service_port(sa_family_t family)
{
	struct sockaddr_in6 addr6 = {
		.sin6_family = AF_INET6,
		.sin6_addr = in6addr_any,
		.sin6_port = htons(SERVICE_PORT),
	};
	struct sockaddr_in addr4 = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(SERVICE_PORT),
	};
	struct sockaddr *addr;
	socklen_t addrlen;
	int sock;

	if (family == AF_INET6) {
		addr = (struct sockaddr *)&addr6;
		addrlen = sizeof(addr6);
	} else {
		addr = (struct sockaddr *)&addr4;
		addrlen = sizeof(addr4);
	}

	sock = zsock_socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		LOG_ERR("Cannot create service socket (%d)", -errno);
		return -errno;
	}

	if (zsock_bind(sock, addr, addrlen) < 0) {
		LOG_ERR("Cannot bind service socket (%d)", -errno);
		zsock_close(sock);
		return -errno;
	}

	if (zsock_listen(sock, 1) < 0) {
		LOG_ERR("Cannot listen on service socket (%d)", -errno);
		zsock_close(sock);
		return -errno;
	}

	return sock;
}

int main(void)
{
	/* The offloaded interface comes up and mirrors the host addresses at
	 * boot; the mDNS responder then runs on its own, without application
	 * interaction. Bind the advertised service port so the responder
	 * answers for it, then signal readiness to the host-side test harness.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		(void)listen_on_service_port(AF_INET6);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		(void)listen_on_service_port(AF_INET);
	}

	LOG_INF("mDNS offload responder ready");

	return 0;
}
