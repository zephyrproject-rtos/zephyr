/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_echo_server_svc_sample, LOG_LEVEL_DBG);

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/net/socket_service.h>

#define MY_PORT 4242

static char addr_str[INET6_ADDRSTRLEN];

static struct pollfd sockfd_udp[1] = {
	[0] = { .fd = -1 }, /* UDP socket */
};
static struct pollfd sockfd_tcp[1] = {
	[0] = { .fd = -1 }, /* TCP socket */
};

#define MAX_SERVICES 1

static void receive_data(bool is_udp, struct net_socket_service_event *pev,
			 char *buf, size_t buflen);

static void tcp_service_handler(struct k_work *work)
{
	struct net_socket_service_event *pev =
		CONTAINER_OF(work, struct net_socket_service_event, work);
	static char buf[1500];

	/* Note that in this application we receive / send data from
	 * system work queue. In proper application the socket reading and data
	 * sending should be done so that the system work queue is not blocked.
	 * It is possible to create a socket service that uses own work queue.
	 */
	receive_data(false, pev, buf, sizeof(buf));
}

static void udp_service_handler(struct k_work *work)
{
	struct net_socket_service_event *pev =
		CONTAINER_OF(work, struct net_socket_service_event, work);
	static char buf[1500];

	receive_data(true, pev, buf, sizeof(buf));
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_udp, NULL, udp_service_handler, MAX_SERVICES);
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_tcp, NULL, tcp_service_handler, MAX_SERVICES);

static void receive_data(bool is_udp, struct net_socket_service_event *pev,
			 char *buf, size_t buflen)
{
	struct pollfd *pfd = &pev->event;
	int client = pfd->fd;
	struct sockaddr_in6 addr;
	socklen_t addrlen = sizeof(addr);
	int len, out_len;
	char *p;

	len = recvfrom(client, buf, buflen, 0,
		       (struct sockaddr *)&addr, &addrlen);
	if (len <= 0) {
		if (len < 0) {
			LOG_ERR("recv: %d", -errno);
		}

		/* If the TCP socket is closed, mark it as non pollable */
		if (!is_udp && sockfd_tcp[0].fd == client) {
			sockfd_tcp[0].fd = -1;

			/* Update the handler so that client connection is
			 * not monitored any more.
			 */
			(void)net_socket_service_register(&service_tcp, sockfd_tcp,
							  ARRAY_SIZE(sockfd_tcp), NULL);
			close(client);

			LOG_INF("Connection from %s closed", addr_str);
		}

		return;
	}

	p = buf;
	do {
		if (is_udp) {
			out_len = sendto(client, p, len, 0,
					 (struct sockaddr *)&addr, addrlen);
		} else {
			out_len = send(client, p, len, 0);
		}

		if (out_len < 0) {
			LOG_ERR("sendto: %d", -errno);
			break;
		}

		p += out_len;
		len -= out_len;
	} while (len);
}

static int setup_tcp_socket(struct sockaddr_in6 *addr)
{
	socklen_t optlen = sizeof(int);
	int ret, sock, opt;

	sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		LOG_ERR("socket: %d", -errno);
		return -errno;
	}

	ret = getsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, &optlen);
	if (ret == 0 && opt) {
		LOG_INF("IPV6_V6ONLY option is on, turning it off.");

		opt = 0;
		ret = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, optlen);
		if (ret < 0) {
			LOG_WRN("Cannot turn off IPV6_V6ONLY option");
		} else {
			LOG_INF("Sharing same socket between IPv6 and IPv4");
		}
	}

	if (bind(sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
		LOG_ERR("bind: %d", -errno);
		return -errno;
	}

	if (listen(sock, 5) < 0) {
		LOG_ERR("listen: %d", -errno);
		return -errno;
	}

	return sock;
}

static int setup_udp_socket(struct sockaddr_in6 *addr)
{
	socklen_t optlen = sizeof(int);
	int ret, sock, opt;

	sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("socket: %d", -errno);
		return -errno;
	}

	ret = getsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, &optlen);
	if (ret == 0 && opt) {
		LOG_INF("IPV6_V6ONLY option is on, turning it off.");

		opt = 0;
		ret = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, optlen);
		if (ret < 0) {
			LOG_WRN("Cannot turn off IPV6_V6ONLY option");
		} else {
			LOG_INF("Sharing same socket between IPv6 and IPv4");
		}
	}

	if (bind(sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
		LOG_ERR("bind: %d", -errno);
		return -errno;
	}

	return sock;
}

int main(void)
{
	int tcp_sock, udp_sock, ret;
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_port = htons(MY_PORT),
	};
	static int counter;

	tcp_sock = setup_tcp_socket(&addr);
	if (tcp_sock < 0) {
		return tcp_sock;
	}

	udp_sock = setup_udp_socket(&addr);
	if (udp_sock < 0) {
		return udp_sock;
	}

	sockfd_udp[0].fd = udp_sock;
	sockfd_udp[0].events = POLLIN;

	/* Register UDP socket to service handler */
	ret = net_socket_service_register(&service_udp, sockfd_udp,
					  ARRAY_SIZE(sockfd_udp), NULL);
	if (ret < 0) {
		LOG_ERR("Cannot register socket service handler (%d)", ret);
	}

	LOG_INF("Single-threaded TCP/UDP echo server waits "
		"for a connection on port %d", MY_PORT);

	while (1) {
		struct sockaddr_in6 client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client;

		client = accept(tcp_sock, (struct sockaddr *)&client_addr,
				&client_addr_len);
		if (client < 0) {
			LOG_ERR("accept: %d", -errno);
			continue;
		}

		inet_ntop(client_addr.sin6_family, &client_addr.sin6_addr,
			  addr_str, sizeof(addr_str));
		LOG_INF("Connection #%d from %s (%d)", counter++, addr_str, client);

		sockfd_tcp[0].fd = client;
		sockfd_tcp[0].events = POLLIN;

		/* Register all the sockets to service handler */
		ret = net_socket_service_register(&service_tcp, sockfd_tcp,
						  ARRAY_SIZE(sockfd_tcp), NULL);
		if (ret < 0) {
			LOG_ERR("Cannot register socket service handler (%d)",
				ret);
			break;
		}
	}

	(void)net_socket_service_unregister(&service_tcp);
	(void)net_socket_service_unregister(&service_udp);

	close(tcp_sock);
	close(udp_sock);

	return 0;
}
