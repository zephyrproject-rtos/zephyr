/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include "tcp_server.h"

LOG_MODULE_REGISTER(mpipe_net, CONFIG_MPIPE_LOG_LEVEL);

int mpipe_net_tcp_listen(uint16_t port)
{
	struct net_sockaddr_in6 addr6 = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(port),
	};
	struct net_sockaddr_in addr4 = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(port),
	};
	struct net_sockaddr *addr;
	net_socklen_t addrlen;
	int family;
	int opt = 1;
	int ret;
	int fd;

	/* With IPv6 on, one socket serves IPv4 clients too through mapped addresses */
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		family = NET_AF_INET6;
		addr = (struct net_sockaddr *)&addr6;
		addrlen = sizeof(addr6);
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		family = NET_AF_INET;
		addr = (struct net_sockaddr *)&addr4;
		addrlen = sizeof(addr4);
	} else {
		return -ENOTSUP;
	}

	fd = zsock_socket(family, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (fd < 0) {
		LOG_ERR("socket() failed (%d)", errno);
		return -errno;
	}

	(void)zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &opt, sizeof(opt));

	if (IS_ENABLED(CONFIG_NET_IPV6) && IS_ENABLED(CONFIG_NET_IPV4)) {
		int v6only = 0;

		(void)zsock_setsockopt(fd, NET_IPPROTO_IPV6, ZSOCK_IPV6_V6ONLY, &v6only,
				       sizeof(v6only));
	}

	if (zsock_bind(fd, addr, addrlen) < 0 || zsock_listen(fd, 1) < 0) {
		ret = -errno;
		LOG_ERR("Failed to listen on port %u (%d)", port, errno);
		(void)zsock_close(fd);
		return ret;
	}

	return fd;
}

int mpipe_net_tcp_accept(int server_fd)
{
	int fd;

	LOG_DBG("Waiting for a client");

	fd = zsock_accept(server_fd, NULL, NULL);
	if (fd < 0) {
		LOG_ERR("accept() failed (%d)", errno);
		return -errno;
	}

	return fd;
}

void mpipe_net_tcp_close(int *fd)
{
	if (*fd >= 0) {
		(void)zsock_close(*fd);
		*fd = -1;
	}
}
