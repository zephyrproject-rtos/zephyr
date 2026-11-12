/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_snmp, CONFIG_SNMP_LOG_LEVEL);

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/snmp.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>

#include "snmp_internal.h"

/* One listening socket per enabled address family. The IPv6 socket keeps
 * the default IPV6_V6ONLY setting so that it does not collide with the IPv4
 * one.
 */
#define SOCKET_COUNT (IS_ENABLED(CONFIG_NET_IPV4) + IS_ENABLED(CONFIG_NET_IPV6))

/* Both buffers are only touched from the socket service thread. */
static uint8_t rx_buf[CONFIG_SNMP_MAX_PDU_SIZE];
static uint8_t tx_buf[CONFIG_SNMP_MAX_PDU_SIZE];

static struct zsock_pollfd fds[SOCKET_COUNT];
static int fd_count;
static int sock_v4 = -1;
static int sock_v6 = -1;
static bool running;
static int64_t start_uptime;
static K_MUTEX_DEFINE(agent_lock);

/* Room for the packet information of either address family. */
#define CMSG_BUF_LEN                                                                               \
	MAX(NET_CMSG_SPACE(sizeof(struct net_in_pktinfo)),                                         \
	    NET_CMSG_SPACE(sizeof(struct net_in6_pktinfo)))

/*
 * Note the local address a request arrived on, so that the response leaves
 * from the same one as RFC 1157 section 4.1 requires. A multicast or
 * broadcast destination is not usable as a source and is left unset.
 */
static void local_address_from_cmsg(const struct net_msghdr *msg,
				    struct net_sockaddr_storage *local)
{
	struct net_cmsghdr *cmsg;

	local->ss_family = NET_AF_UNSPEC;

	for (cmsg = NET_CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = NET_CMSG_NXTHDR(msg, cmsg)) {
		if (IS_ENABLED(CONFIG_NET_IPV6) && cmsg->cmsg_level == NET_IPPROTO_IPV6 &&
		    cmsg->cmsg_type == ZSOCK_IPV6_PKTINFO &&
		    cmsg->cmsg_len == NET_CMSG_LEN(sizeof(struct net_in6_pktinfo))) {
			const struct net_in6_pktinfo *info = (void *)NET_CMSG_DATA(cmsg);
			struct net_sockaddr_in6 *addr6 = net_sin6(net_sad(local));

			if (!net_ipv6_is_addr_mcast(&info->ipi6_addr)) {
				addr6->sin6_family = NET_AF_INET6;
				addr6->sin6_addr = info->ipi6_addr;
			}
		} else if (IS_ENABLED(CONFIG_NET_IPV4) && cmsg->cmsg_level == NET_IPPROTO_IP &&
			   cmsg->cmsg_type == ZSOCK_IP_PKTINFO &&
			   cmsg->cmsg_len == NET_CMSG_LEN(sizeof(struct net_in_pktinfo))) {
			const struct net_in_pktinfo *info = (void *)NET_CMSG_DATA(cmsg);
			struct net_sockaddr_in *addr4 = net_sin(net_sad(local));
			struct net_if *iface = net_if_get_by_index((int)info->ipi_ifindex);

			if (!net_ipv4_is_addr_mcast(&info->ipi_addr) &&
			    !net_ipv4_is_addr_bcast(iface, &info->ipi_addr)) {
				addr4->sin_family = NET_AF_INET;
				addr4->sin_addr = info->ipi_addr;
			}
		}
	}
}

/* Send from local, or let the stack choose when no local address is known. */
static int send_response(int sock, const void *data, size_t len, const struct net_sockaddr *dst,
			 net_socklen_t dst_len, const struct net_sockaddr_storage *local)
{
	union {
		struct net_cmsghdr hdr;
		uint8_t buf[CMSG_BUF_LEN];
	} cmsg_storage = {0};
	struct net_cmsghdr *cmsg = &cmsg_storage.hdr;
	struct net_iovec iov = {.iov_base = (void *)data, .iov_len = len};
	struct net_msghdr msg = {
		.msg_name = (void *)dst,
		.msg_namelen = dst_len,
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = cmsg_storage.buf,
		.msg_controllen = sizeof(cmsg_storage.buf),
	};

	if (IS_ENABLED(CONFIG_NET_IPV6) && local->ss_family == NET_AF_INET6) {
		struct net_in6_pktinfo info = {
			.ipi6_addr = net_sin6(net_sad(local))->sin6_addr,
		};

		cmsg->cmsg_level = NET_IPPROTO_IPV6;
		cmsg->cmsg_type = ZSOCK_IPV6_PKTINFO;
		cmsg->cmsg_len = NET_CMSG_LEN(sizeof(info));
		memcpy(NET_CMSG_DATA(cmsg), &info, sizeof(info));
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && local->ss_family == NET_AF_INET) {
		struct net_in_pktinfo info = {
			.ipi_spec_dst = net_sin(net_sad(local))->sin_addr,
		};

		cmsg->cmsg_level = NET_IPPROTO_IP;
		cmsg->cmsg_type = ZSOCK_IP_PKTINFO;
		cmsg->cmsg_len = NET_CMSG_LEN(sizeof(info));
		memcpy(NET_CMSG_DATA(cmsg), &info, sizeof(info));
	} else {
		return zsock_sendto(sock, data, len, 0, dst, dst_len);
	}

	return zsock_sendmsg(sock, &msg, 0);
}

static void snmp_service_handler(struct net_socket_service_event *pev)
{
	struct zsock_pollfd *pfd = &pev->event;
	struct net_sockaddr_storage addr, local;
	/* The stack places the packet information in the first zeroed slot and
	 * reads back whatever headers it finds, so the buffer must start out
	 * clear and be aligned for struct net_cmsghdr.
	 */
	union {
		struct net_cmsghdr hdr;
		uint8_t buf[CMSG_BUF_LEN];
	} cmsg_storage = {0};
	struct net_iovec iov = {.iov_base = rx_buf, .iov_len = sizeof(rx_buf)};
	struct net_msghdr msg = {
		.msg_name = &addr,
		.msg_namelen = sizeof(addr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = cmsg_storage.buf,
		.msg_controllen = sizeof(cmsg_storage.buf),
	};
	size_t rsp_start, rsp_len;
	int ret;

	if ((pfd->revents & ZSOCK_POLLIN) == 0) {
		LOG_ERR("Socket %d not readable, revents 0x%x", pfd->fd, pfd->revents);
		return;
	}

	ret = zsock_recvmsg(pfd->fd, &msg, 0);
	if (ret < 0) {
		LOG_ERR("Receive error %d", errno);
		return;
	}

	local_address_from_cmsg(&msg, &local);

	rsp_len = snmp_process_message(rx_buf, (size_t)ret, tx_buf, sizeof(tx_buf), &rsp_start);
	if (rsp_len == 0U) {
		return;
	}

	ret = send_response(pfd->fd, &tx_buf[rsp_start], rsp_len, net_sad(&addr), msg.msg_namelen,
			    &local);
	if (ret < 0) {
		LOG_ERR("Send error %d", errno);
	}
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(snmp_service, snmp_service_handler, SOCKET_COUNT);

static int open_socket(const struct net_sockaddr *addr, net_socklen_t addrlen)
{
	int on = 1;
	int sock, ret;

	sock = zsock_socket(addr->sa_family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	if (sock < 0) {
		ret = -errno;
		LOG_ERR("Socket error %d", ret);
		return ret;
	}

	if (zsock_bind(sock, addr, addrlen) < 0) {
		ret = -errno;
		LOG_ERR("Bind error %d", ret);
		(void)zsock_close(sock);
		return ret;
	}

	/* Ask for the destination address of each request; without it the
	 * stack picks the source of the response.
	 */
	if (addr->sa_family == NET_AF_INET6) {
		ret = zsock_setsockopt(sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_RECVPKTINFO, &on,
				       sizeof(on));
	} else {
		ret = zsock_setsockopt(sock, NET_IPPROTO_IP, ZSOCK_IP_PKTINFO, &on, sizeof(on));
	}

	if (ret < 0) {
		LOG_WRN("Packet info unavailable, responses use the default source address");
	}

	return sock;
}

int snmp_agent_start(void)
{
	int count = 0;
	int ret;

	k_mutex_lock(&agent_lock, K_FOREVER);

	if (running) {
		ret = -EALREADY;
		goto out;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		struct net_sockaddr_in addr4 = {
			.sin_family = NET_AF_INET,
			.sin_addr = NET_INADDR_ANY_INIT,
			.sin_port = net_htons(CONFIG_SNMP_PORT),
		};

		ret = open_socket((struct net_sockaddr *)&addr4, sizeof(addr4));
		if (ret < 0) {
			goto cleanup;
		}

		sock_v4 = ret;
		fds[count++] = (struct zsock_pollfd){.fd = ret, .events = ZSOCK_POLLIN};
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		struct net_sockaddr_in6 addr6 = {
			.sin6_family = NET_AF_INET6,
			.sin6_addr = NET_IN6ADDR_ANY_INIT,
			.sin6_port = net_htons(CONFIG_SNMP_PORT),
		};

		ret = open_socket((struct net_sockaddr *)&addr6, sizeof(addr6));
		if (ret < 0) {
			goto cleanup;
		}

		sock_v6 = ret;
		fds[count++] = (struct zsock_pollfd){.fd = ret, .events = ZSOCK_POLLIN};
	}

	ret = net_socket_service_register(&snmp_service, fds, count, NULL);
	if (ret < 0) {
		LOG_ERR("Socket service registration failed %d", ret);
		goto cleanup;
	}

	fd_count = count;
	running = true;
	start_uptime = k_uptime_get();
	LOG_DBG("Agent listening on port %d", CONFIG_SNMP_PORT);
	k_mutex_unlock(&agent_lock);

	/* RFC 3418: the agent announces its (re)initialization. */
	if (IS_ENABLED(CONFIG_SNMP_NOTIFICATIONS)) {
		snmp_notify_cold_start();
	}

	return 0;

cleanup:
	for (int i = 0; i < count; i++) {
		(void)zsock_close(fds[i].fd);
	}

	sock_v4 = -1;
	sock_v6 = -1;

out:
	k_mutex_unlock(&agent_lock);

	return ret;
}

int snmp_transport_send(const struct net_sockaddr *dst, net_socklen_t dst_len, const void *data,
			size_t len)
{
	int sock;

	if (!running) {
		return -ENOTCONN;
	}

	sock = (dst->sa_family == NET_AF_INET6) ? sock_v6 : sock_v4;
	if (sock < 0) {
		return -EAFNOSUPPORT;
	}

	if (zsock_sendto(sock, data, len, 0, dst, dst_len) < 0) {
		return -errno;
	}

	return 0;
}

int64_t snmp_agent_start_uptime(void)
{
	return start_uptime;
}

int snmp_agent_stop(void)
{
	int ret = 0;

	k_mutex_lock(&agent_lock, K_FOREVER);

	if (!running) {
		ret = -EALREADY;
		goto out;
	}

	/* Unregister first, then close here rather than through the service,
	 * which would do so asynchronously: the port must be free again when
	 * this returns so that the agent can be restarted at once.
	 */
	ret = net_socket_service_unregister(&snmp_service);
	if (ret < 0) {
		LOG_ERR("Socket service unregister failed %d", ret);
	}

	for (int i = 0; i < fd_count; i++) {
		(void)zsock_close(fds[i].fd);
	}

	fd_count = 0;
	sock_v4 = -1;
	sock_v6 = -1;
	running = false;

out:
	k_mutex_unlock(&agent_lock);

	return ret;
}
