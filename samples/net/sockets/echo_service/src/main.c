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

#define MAX_SERVICES (CONFIG_ZVFS_POLL_MAX - 3)

BUILD_ASSERT(MAX_SERVICES > 0, "Need at least 4 poll fds, increase CONFIG_ZVFS_POLL_MAX");

K_MUTEX_DEFINE(lock);
static struct pollfd sockfd_tcp[MAX_SERVICES];
static int tcp_socket = -1;
static int udp_socket = -1;

static void receive_data(bool is_udp, struct net_socket_service_event *pev,
			 char *buf, size_t buflen);
static void tcp_accept_handler(struct net_socket_service_event *pev);
static void restart_echo_service(void);

static void tcp_service_handler(struct net_socket_service_event *pev)
{
	static char buf[1500];

	receive_data(false, pev, buf, sizeof(buf));
}

static void udp_service_handler(struct net_socket_service_event *pev)
{
	static char buf[1500];

	receive_data(true, pev, buf, sizeof(buf));
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_udp, udp_service_handler, 1);
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_accept, tcp_accept_handler, 1);
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_tcp, tcp_service_handler, MAX_SERVICES);

static void client_list_init(void)
{
	k_mutex_lock(&lock, K_FOREVER);
	ARRAY_FOR_EACH_PTR(sockfd_tcp, sockfd) {
		sockfd->fd = -1;
	}
	k_mutex_unlock(&lock);
}

static void client_list_deinit(void)
{
	(void)net_socket_service_unregister(&service_tcp);

	k_mutex_lock(&lock, K_FOREVER);
	ARRAY_FOR_EACH_PTR(sockfd_tcp, sockfd) {
		if (sockfd->fd != -1) {
			close(sockfd->fd);
			sockfd->fd = -1;
		}
	}
	k_mutex_unlock(&lock);
}

static void client_list_try_add(int sock)
{
	int ret;

	k_mutex_lock(&lock, K_FOREVER);
	ARRAY_FOR_EACH_PTR(sockfd_tcp, sockfd) {
		if (sockfd->fd == -1) {
			sockfd->fd = sock;
			sockfd->events = POLLIN;

			ret = net_socket_service_register(&service_tcp, sockfd_tcp,
							  ARRAY_SIZE(sockfd_tcp), NULL);
			if (ret < 0) {
				LOG_ERR("Cannot register socket service handler (%d). "
					"Attempting to restart service.", ret);
				restart_echo_service();
			}
			k_mutex_unlock(&lock);
			return;
		}
	}
	k_mutex_unlock(&lock);
	LOG_WRN("Max capacity reached");
	close(sock);
}

static void client_list_remove(int client)
{
	int ret;

	k_mutex_lock(&lock, K_FOREVER);
	ARRAY_FOR_EACH_PTR(sockfd_tcp, sockfd) {
		if (sockfd->fd == client) {
			close(client);
			sockfd->fd = -1;
			ret = net_socket_service_register(&service_tcp, sockfd_tcp,
							  ARRAY_SIZE(sockfd_tcp), NULL);
			if (ret < 0) {
				LOG_ERR("Cannot register socket service handler (%d). "
					"Attempting to restart service.", ret);
				restart_echo_service();
			}
			break;
		}
	}
	k_mutex_unlock(&lock);
}

static void receive_data(bool is_udp, struct net_socket_service_event *pev,
			 char *buf, size_t buflen)
{
	struct pollfd *pfd = &pev->event;
	int client = pfd->fd;
	struct sockaddr_in6 addr;
	char addr_str[INET6_ADDRSTRLEN];
	socklen_t addrlen = sizeof(addr);
	int len, out_len;
	char *p;

	len = recvfrom(client, buf, buflen, 0,
		       (struct sockaddr *)&addr, &addrlen);
	if (len <= 0) {
		if (len < 0) {
			LOG_ERR("recv: %d", -errno);
		}
		if (!is_udp) {
			client_list_remove(client);
			inet_ntop(addr.sin6_family, &addr.sin6_addr, addr_str, sizeof(addr_str));
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

static void tcp_accept_handler(struct net_socket_service_event *pev)
{
	int client;
	int sock = pev->event.fd;
	char addr_str[INET6_ADDRSTRLEN];
	struct sockaddr_in6 client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	client = accept(sock, (struct sockaddr *)&client_addr, &client_addr_len);
	if (client < 0) {
		LOG_ERR("accept: %d. Restarting service.", -errno);
		restart_echo_service();
		return;
	}

	inet_ntop(client_addr.sin6_family, &client_addr.sin6_addr, addr_str, sizeof(addr_str));
	LOG_INF("Connection from %s (%d)", addr_str, client);

	client_list_try_add(client);
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
		close(sock);
		return -errno;
	}

	if (listen(sock, 5) < 0) {
		LOG_ERR("listen: %d", -errno);
		close(sock);
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
		close(sock);
		return -errno;
	}

	return sock;
}

static int start_echo_service(void)
{
	struct pollfd sockfd_accept, sockfd_udp;
	int tcp_sock, udp_sock, ret;
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_port = htons(MY_PORT),
	};

	client_list_init();

	tcp_sock = setup_tcp_socket(&addr);
	if (tcp_sock < 0) {
		LOG_ERR("Failed to setup tcp listening socket");
		return tcp_sock;
	}

	udp_sock = setup_udp_socket(&addr);
	if (udp_sock < 0) {
		LOG_ERR("Failed to setup udp socket");
		ret = udp_sock;
		goto cleanup_tcp;
	}

	/* Register TCP listening socket to service handler */
	sockfd_accept = (struct pollfd){ .fd = tcp_sock, .events = POLLIN };
	ret = net_socket_service_register(&service_accept, &sockfd_accept, 1, NULL);
	if (ret < 0) {
		LOG_ERR("Cannot register socket service handler (%d)", ret);
		goto cleanup_sockets;
	}

	/* Register UDP socket to service handler */
	sockfd_udp = (struct pollfd){ .fd = udp_sock, .events = POLLIN };
	ret = net_socket_service_register(&service_udp, &sockfd_udp, 1, NULL);
	if (ret < 0) {
		LOG_ERR("Cannot register socket service handler (%d)", ret);
		goto cleanup_sockets;
	}

	k_mutex_lock(&lock, K_FOREVER);
	tcp_socket = tcp_sock;
	udp_socket = udp_sock;
	k_mutex_unlock(&lock);

	LOG_INF("Single-threaded TCP/UDP echo server waits "
		"for a connection on port %d", MY_PORT);

	return 0;

cleanup_sockets:
	close(udp_sock);
cleanup_tcp:
	close(tcp_sock);
	return -1;

}

static int stop_echo_service(void)
{
	client_list_deinit();
	(void)net_socket_service_unregister(&service_udp);
	(void)net_socket_service_unregister(&service_accept);

	k_mutex_lock(&lock, K_FOREVER);
	if (tcp_socket >= 0) {
		close(tcp_socket);
		tcp_socket = -1;
	}
	if (udp_socket >= 0) {
		close(udp_socket);
		udp_socket = -1;
	}
	k_mutex_unlock(&lock);

	return 0;
}

static void restart_echo_service(void)
{
	stop_echo_service();
	start_echo_service();
}

SYS_INIT(start_echo_service, APPLICATION, 99);
