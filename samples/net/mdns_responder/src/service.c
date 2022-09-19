/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/net/dns_sd.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdns_echo_service, LOG_LEVEL_DBG);

/* A default port of 0 causes bind(2) to request an ephemeral port */
#define DEFAULT_PORT 0

static int welcome(int fd)
{
	static const char msg[] = "Bonjour, Zephyr world!\n";

	return send(fd, msg, sizeof(msg), 0);
}

/* This is mainly here to bind to a port to get service advertisement
 * to work.. but since we're already here we might as well do something
 * useful.
 */
void service(void)
{
	int r;
	int server_fd;
	int client_fd;
	socklen_t len;
	void *addrp;
	uint16_t *portp;
	struct sockaddr client_addr;
	char addrstr[INET6_ADDRSTRLEN];
	uint8_t line[64];

	static struct sockaddr server_addr;

#if DEFAULT_PORT == 0
	/* The advanced use case: ephemeral port */
#if defined(CONFIG_NET_IPV6)
	DNS_SD_REGISTER_SERVICE(zephyr, CONFIG_NET_HOSTNAME,
				"_zephyr", "_tcp", "local", DNS_SD_EMPTY_TXT,
				&((struct sockaddr_in6 *)&server_addr)->sin6_port);
#elif defined(CONFIG_NET_IPV4)
	DNS_SD_REGISTER_SERVICE(zephyr, CONFIG_NET_HOSTNAME,
				"_zephyr", "_tcp", "local", DNS_SD_EMPTY_TXT,
				&((struct sockaddr_in *)&server_addr)->sin_port);
#endif
#else
	/* The simple use case: fixed port */
	DNS_SD_REGISTER_TCP_SERVICE(zephyr, CONFIG_NET_HOSTNAME,
				    "_zephyr", "local", DNS_SD_EMPTY_TXT, DEFAULT_PORT);
#endif

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_sin6(&server_addr)->sin6_family = AF_INET6;
		net_sin6(&server_addr)->sin6_addr = in6addr_any;
		net_sin6(&server_addr)->sin6_port = sys_cpu_to_be16(DEFAULT_PORT);
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_sin(&server_addr)->sin_family = AF_INET;
		net_sin(&server_addr)->sin_addr.s_addr = htonl(INADDR_ANY);
		net_sin(&server_addr)->sin_port = sys_cpu_to_be16(DEFAULT_PORT);
	} else {
		__ASSERT(false, "Neither IPv6 nor IPv4 are enabled");
	}

	r = socket(server_addr.sa_family, SOCK_STREAM, 0);
	if (r == -1) {
		NET_DBG("socket() failed (%d)", errno);
		return;
	}

	server_fd = r;
	NET_DBG("server_fd is %d", server_fd);

	r = bind(server_fd, &server_addr, sizeof(server_addr));
	if (r == -1) {
		NET_DBG("bind() failed (%d)", errno);
		close(server_fd);
		return;
	}

	if (server_addr.sa_family == AF_INET6) {
		addrp = &net_sin6(&server_addr)->sin6_addr;
		portp = &net_sin6(&server_addr)->sin6_port;
	} else {
		addrp = &net_sin(&server_addr)->sin_addr;
		portp = &net_sin(&server_addr)->sin_port;
	}

	inet_ntop(server_addr.sa_family, addrp, addrstr, sizeof(addrstr));
	NET_DBG("bound to [%s]:%u",
		addrstr, ntohs(*portp));

	r = listen(server_fd, 1);
	if (r == -1) {
		NET_DBG("listen() failed (%d)", errno);
		close(server_fd);
		return;
	}

	for (;;) {
		len = sizeof(client_addr);
		r = accept(server_fd, (struct sockaddr *)&client_addr, &len);
		if (r == -1) {
			NET_DBG("accept() failed (%d)", errno);
			continue;
		}

		client_fd = r;

		inet_ntop(server_addr.sa_family, addrp, addrstr, sizeof(addrstr));
		NET_DBG("accepted connection from [%s]:%u",
			addrstr, ntohs(*portp));

		/* send a banner */
		r = welcome(client_fd);
		if (r == -1) {
			NET_DBG("send() failed (%d)", errno);
			close(client_fd);
			return;
		}

		for (;;) {
			/* echo 1 line at a time */
			r = recv(client_fd, line, sizeof(line), 0);
			if (r == -1) {
				NET_DBG("recv() failed (%d)", errno);
				close(client_fd);
				break;
			}
			len = r;

			r = send(client_fd, line, len, 0);
			if (r == -1) {
				NET_DBG("send() failed (%d)", errno);
				close(client_fd);
				break;
			}
		}
	}
}
