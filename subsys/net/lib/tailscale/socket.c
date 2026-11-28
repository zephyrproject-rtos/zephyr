/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/fdtable.h>

#include "ingress.h"
#include "policy.h"
#include "socket.h"

static int connect_until(int fd, const struct net_sockaddr *remote, net_socklen_t len,
			 int64_t deadline)
{
	int64_t remaining = deadline - k_uptime_get();
	struct zsock_pollfd pfd = {.fd = fd, .events = ZSOCK_POLLOUT};
	net_socklen_t error_len = sizeof(int);
	int error;
	int rc;

	if (remaining <= 0) {
		return -ETIMEDOUT;
	}
	if (zsock_connect(fd, remote, len) == 0) {
		return 0;
	}
	if (errno != EINPROGRESS) {
		return -errno;
	}
	remaining = deadline - k_uptime_get();
	if (remaining <= 0) {
		return -ETIMEDOUT;
	}
	rc = zsock_poll(&pfd, 1, (int)remaining);
	if (rc <= 0) {
		return rc < 0 ? -errno : -ETIMEDOUT;
	}
	if (zsock_getsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_ERROR, &error, &error_len) < 0) {
		return -errno;
	}
	if (error != 0) {
		return -error;
	}
	return (pfd.revents & ZSOCK_POLLOUT) != 0 ? 0 : -ECONNABORTED;
}

int tailscale_socket_connect_dns(int type, const struct net_sockaddr *remote, net_socklen_t len,
			      uint64_t route_revision, int32_t timeout_ms)
{
	struct net_sockaddr_storage local;
	struct net_if *iface;
	uint64_t revision;
	int64_t deadline = k_uptime_get() + timeout_ms;
	uint8_t protocol;
	int fd;
	int rc;

	if (remote == NULL || len < sizeof(remote->sa_family) || timeout_ms <= 0 ||
	    (type != NET_SOCK_STREAM && type != NET_SOCK_DGRAM)) {
		return -EINVAL;
	}
	switch (remote->sa_family) {
	case NET_AF_INET:
		if (len < sizeof(struct net_sockaddr_in) || net_sin(remote)->sin_port == 0U) {
			return -EINVAL;
		}
		break;
	case NET_AF_INET6:
		if (len < sizeof(struct net_sockaddr_in6) || net_sin6(remote)->sin6_port == 0U) {
			return -EINVAL;
		}
		break;
	default:
		return -EAFNOSUPPORT;
	}

	rc = tailscale_client_policy_route_dns(remote, &iface, &local, &revision);
	if (rc != 0) {
		return rc;
	}
	if (route_revision != revision) {
		return -ESTALE;
	}
	protocol = type == NET_SOCK_STREAM ? NET_IPPROTO_TCP : NET_IPPROTO_UDP;
	fd = zsock_socket(remote->sa_family, type, protocol);
	if (fd < 0) {
		return -errno;
	}
	rc = tailscale_ingress_bind(iface, fd, net_sad(&local));
	if (rc == 0 && zsock_fcntl(fd, ZVFS_F_SETFL, ZVFS_O_NONBLOCK) < 0) {
		rc = -errno;
	}
	if (rc == 0 && revision != tailscale_client_policy_route_revision()) {
		rc = -ESTALE;
	}
	if (rc == 0) {
		rc = connect_until(fd, remote, len, deadline);
	}
	if (rc == 0 && revision != tailscale_client_policy_route_revision()) {
		rc = -ESTALE;
	}
	if (rc == 0 && zsock_fcntl(fd, ZVFS_F_SETFL, 0) < 0) {
		rc = -errno;
	}
	if (rc != 0) {
		if (type == NET_SOCK_STREAM) {
			tailscale_ingress_abort_tcp(fd);
		} else {
			(void)zsock_close(fd);
		}
		return rc;
	}
	return fd;
}

int tailscale_socket_close(int fd)
{
	return zsock_close(fd) < 0 ? -errno : 0;
}
