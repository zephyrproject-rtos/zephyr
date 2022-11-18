/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

LOG_MODULE_DECLARE(net_zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

int zperf_prepare_upload_sock(const struct sockaddr *peer_addr, int tos,
			      int proto)
{
	socklen_t addrlen = peer_addr->sa_family == AF_INET6 ?
			    sizeof(struct sockaddr_in6) :
			    sizeof(struct sockaddr_in);
	int type = (proto == IPPROTO_UDP) ? SOCK_DGRAM : SOCK_STREAM;
	int sock = -1;
	int ret;

	switch (peer_addr->sa_family) {
	case AF_INET:
		if (!IS_ENABLED(CONFIG_NET_IPV4)) {
			NET_ERR("IPv4 not available.");
			return -EINVAL;
		}

		sock = zsock_socket(AF_INET, type, proto);
		if (sock < 0) {
			NET_ERR("Cannot create IPv4 network socket (%d)",
				errno);
			return -errno;
		}

		if (tos > 0) {
			if (zsock_setsockopt(sock, IPPROTO_IP, IP_TOS,
					     &tos, sizeof(tos)) != 0) {
				NET_WARN("Failed to set IP_TOS socket option. "
					 "Please enable CONFIG_NET_CONTEXT_DSCP_ECN.");
			}
		}

		break;

	case AF_INET6:
		if (!IS_ENABLED(CONFIG_NET_IPV6)) {
			NET_ERR("IPv6 not available.");
			return -EINVAL;
		}

		sock = zsock_socket(AF_INET6, type, proto);
		if (sock < 0) {
			NET_ERR("Cannot create IPv6 network socket (%d)",
				errno);
			return -errno;
		}

		if (tos >= 0) {
			if (zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_TCLASS,
					     &tos, sizeof(tos)) != 0) {
				NET_WARN("Failed to set IPV6_TCLASS socket option. "
					 "Please enable CONFIG_NET_CONTEXT_DSCP_ECN.");
			}
		}

		break;

	default:
		LOG_ERR("Invalid address family (%d)", peer_addr->sa_family);
		return -EINVAL;
	}

	ret = zsock_connect(sock, peer_addr, addrlen);
	if (ret < 0) {
		NET_ERR("Connect failed (%d)", errno);
		ret = -errno;
		zsock_close(sock);
		return ret;
	}

	return sock;
}

uint32_t zperf_packet_duration(uint32_t packet_size, uint32_t rate_in_kbps)
{
	return (uint32_t)(((uint64_t)packet_size * 8U * USEC_PER_SEC) /
			  (rate_in_kbps * 1024U));
}
