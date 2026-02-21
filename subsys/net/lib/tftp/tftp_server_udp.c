/*
 * Copyright (c) 2026 Guy Shilman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tftp_server, CONFIG_TFTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/tftp_server.h>
#include <zephyr/net/tftp_common.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/fcntl.h>

/**
 * @brief UDP transport context
 */
struct tftp_udp_transport {
	struct {
		int fd;
		net_sa_family_t family;
	} sockets[2];
	struct zsock_pollfd fds[2];
	size_t sock_count;
	uint16_t port;
};

static int tftp_udp_set_nonblocking(int sock)
{
	int flags;
	int ret;

	flags = zsock_fcntl(sock, F_GETFL, 0);
	if (flags < 0) {
		return -errno;
	}

	ret = zsock_fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	if (ret < 0) {
		return -errno;
	}

	return 0;
}

static int tftp_udp_add_socket(struct tftp_udp_transport *udp, net_sa_family_t family,
			       uint16_t port)
{
	int fd;
	int ret;
	int reuse = 1;

	if (udp->sock_count >= ARRAY_SIZE(udp->sockets)) {
		return -ENOSPC;
	}

	fd = zsock_socket(family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	if (fd < 0) {
		return -errno;
	}

	/* Enable SO_REUSEADDR to allow port reuse after server restart */
	ret = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &reuse, sizeof(reuse));
	if (ret < 0) {
		ret = -errno;
		zsock_close(fd);
		return ret;
	}

	/* Enable SO_REUSEPORT if available for better port reuse */
#ifdef ZSOCK_SO_REUSEPORT
	int reuseport = 1;

	ret = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEPORT, &reuseport,
			       sizeof(reuseport));
	if (ret < 0) {
		NET_DBG("SO_REUSEPORT not supported, continuing with SO_REUSEADDR only");
	}
#endif

	if (family == NET_AF_INET) {
		struct net_sockaddr_in addr4 = {
			.sin_family = NET_AF_INET,
			.sin_port = net_htons(port),
			.sin_addr = {.s_addr = NET_ADDR_ANY},
		};

		ret = zsock_bind(fd, (struct net_sockaddr *)&addr4, sizeof(addr4));
	} else {
		struct net_sockaddr_in6 addr6 = {
			.sin6_family = NET_AF_INET6,
			.sin6_port = net_htons(port),
			.sin6_addr = NET_IN6ADDR_ANY_INIT,
		};

		ret = zsock_bind(fd, (struct net_sockaddr *)&addr6, sizeof(addr6));
	}

	if (ret < 0) {
		ret = -errno;
		zsock_close(fd);
		return ret;
	}

	ret = tftp_udp_set_nonblocking(fd);
	if (ret < 0) {
		zsock_close(fd);
		return ret;
	}

	udp->sockets[udp->sock_count].fd = fd;
	udp->sockets[udp->sock_count].family = family;
	udp->fds[udp->sock_count].fd = fd;
	udp->fds[udp->sock_count].events = ZSOCK_POLLIN;
	udp->fds[udp->sock_count].revents = 0;
	udp->sock_count++;

	return 0;
}

/**
 * @brief Initialize UDP transport
 */
static int tftp_udp_init(void *transport, const char *port_str)
{
	struct tftp_udp_transport *udp = (struct tftp_udp_transport *)transport;
	int ret;
	uint16_t port;

	memset(udp, 0, sizeof(*udp));

	if (port_str == NULL) {
		port = TFTP_PORT; /* Default TFTP port */
	} else {
		port = (uint16_t)atoi(port_str);
		if (port == 0) {
			return -EINVAL;
		}
	}

	udp->port = port;

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = tftp_udp_add_socket(udp, NET_AF_INET, port);
		if (ret < 0) {
			NET_DBG("Failed to initialize IPv4 UDP socket: %d", ret);
		}
	}

	if (udp->sock_count == 0) {
		return -EIO;
	}

	NET_INFO("TFTP UDP server listening on all available interfaces, port %u", port);

	return 0;
}

/**
 * @brief UDP transport receive
 */
static int tftp_udp_recv(void *transport, uint8_t *buf, size_t len, struct net_sockaddr *from_addr,
			 net_socklen_t *from_len)
{
	struct tftp_udp_transport *udp = (struct tftp_udp_transport *)transport;
	int ret;

	int poll_ret = zsock_poll(udp->fds, udp->sock_count, 0);

	if (poll_ret == 0) {
		return -EAGAIN;
	}
	if (poll_ret < 0) {
		return -errno;
	}

	for (int i = 0; i < udp->sock_count; i++) {
		if (!(udp->fds[i].revents & (ZSOCK_POLLIN | ZSOCK_POLLERR | ZSOCK_POLLHUP))) {
			continue;
		}

		ret = zsock_recvfrom(udp->fds[i].fd, buf, len, 0, (struct net_sockaddr *)from_addr,
				     from_len);
		if (ret >= 0) {
			return ret;
		}

		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			return -errno;
		}
	}

	return -EAGAIN;
}

/**
 * @brief UDP transport poll
 */
static int tftp_udp_poll(void *transport, int timeout_ms)
{
	struct tftp_udp_transport *udp = (struct tftp_udp_transport *)transport;
	int ret;

	ret = zsock_poll(udp->fds, udp->sock_count, timeout_ms);
	if (ret < 0) {
		return -errno;
	}

	return ret;
}

/**
 * @brief UDP transport send
 */
static int tftp_udp_send(void *transport, const uint8_t *buf, size_t len,
			 const struct net_sockaddr *to_addr, net_socklen_t to_len)
{
	struct tftp_udp_transport *udp = (struct tftp_udp_transport *)transport;
	int sock = -1;

	if (to_addr == NULL) {
		return -EINVAL;
	}

	if (to_addr->sa_family != NET_AF_INET && to_addr->sa_family != NET_AF_INET6) {
		return -EAFNOSUPPORT;
	}

	for (int i = 0; i < udp->sock_count; i++) {
		if (udp->sockets[i].family == to_addr->sa_family) {
			sock = udp->sockets[i].fd;
			break;
		}
	}

	if (sock < 0) {
		return -ENOTSUP;
	}

	return zsock_sendto(sock, buf, len, 0, (const struct net_sockaddr *)to_addr, to_len);
}

/**
 * @brief UDP transport cleanup
 */
static void tftp_udp_cleanup(void *transport)
{
	struct tftp_udp_transport *udp = (struct tftp_udp_transport *)transport;

	for (size_t i = 0; i < udp->sock_count; i++) {
		if (udp->sockets[i].fd >= 0) {
			zsock_close(udp->sockets[i].fd);
			udp->sockets[i].fd = -1;
			udp->fds[i].fd = -1;
		}
	}

	udp->sock_count = 0;
}

/* UDP transport operations */
struct tftp_transport_ops tftp_udp_ops = {
	.init = tftp_udp_init,
	.recv = tftp_udp_recv,
	.send = tftp_udp_send,
	.cleanup = tftp_udp_cleanup,
	.poll = tftp_udp_poll,
};

int tftp_server_init_udp(struct tftp_server *server, const char *port,
			 tftp_server_error_cb_t server_error_cb,
			 tftp_session_error_cb_t session_error_cb)
{
	static struct tftp_udp_transport udp_transport;

	return tftp_server_init(server, &tftp_udp_ops, &udp_transport, port, server_error_cb,
				session_error_cb);
}
