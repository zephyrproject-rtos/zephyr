/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include "tcp_server.h"

LOG_MODULE_REGISTER(mp_net_tcp_server, CONFIG_MP_LOG_LEVEL);

static int tcp_server_bind(uint16_t port)
{
	int opt = 1;
	int fd = -1;
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		struct sockaddr_in6 addr = {
			.sin6_family = AF_INET6,
			.sin6_port = htons(port),
			.sin6_addr = in6addr_any,
		};

		fd = zsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if (fd < 0) {
			LOG_ERR("socket() failed (%d)", errno);
			return -errno;
		}

		/* Clear V6ONLY so IPv4-mapped clients land on the same socket */
		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			int v6only = 0;

			(void)zsock_setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only,
					       sizeof(v6only));
		}

		(void)zsock_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (zsock_bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
			return fd;
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		struct sockaddr_in addr = {
			.sin_family = AF_INET,
			.sin_port = htons(port),
			.sin_addr.s_addr = INADDR_ANY,
		};

		fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (fd < 0) {
			LOG_ERR("socket() failed (%d)", errno);
			return -errno;
		}

		(void)zsock_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (zsock_bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
			return fd;
		}
	} else {
		LOG_ERR("Neither IPv4 nor IPv6 is configured");
		return -ENOTSUP;
	}

	ret = -errno;
	LOG_ERR("bind() on port %u failed (%d)", port, errno);
	(void)zsock_close(fd);

	return ret;
}

int mp_net_tcp_accept_one(uint16_t port, int *server_fd, int *client_fd)
{
	int fd;
	int ret;

	if (server_fd == NULL || client_fd == NULL) {
		return -EINVAL;
	}

	fd = tcp_server_bind(port);
	if (fd < 0) {
		return fd;
	}

	if (zsock_listen(fd, 1) < 0) {
		ret = -errno;
		LOG_ERR("listen() on port %u failed (%d)", port, errno);
		(void)zsock_close(fd);
		return ret;
	}

	LOG_DBG("Waiting for a client on port %u", port);

	*client_fd = zsock_accept(fd, NULL, NULL);
	if (*client_fd < 0) {
		ret = -errno;
		LOG_ERR("accept() on port %u failed (%d)", port, errno);
		(void)zsock_close(fd);
		*client_fd = -1;
		return ret;
	}

	*server_fd = fd;
	LOG_DBG("Client connected on port %u", port);

	return 0;
}

void mp_net_tcp_close(int *server_fd, int *client_fd)
{
	if (*client_fd >= 0) {
		(void)zsock_close(*client_fd);
		*client_fd = -1;
	}

	if (*server_fd >= 0) {
		(void)zsock_close(*server_fd);
		*server_fd = -1;
	}
}
