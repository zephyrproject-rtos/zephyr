/*
 * Copyright (c) 2022 René Beckmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_sn_transport_udp.c
 *
 * @brief MQTT-SN UDP transport.
 */

#include <errno.h>

#include <zephyr/net/mqtt_sn.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/igmp.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_mqtt_sn, CONFIG_MQTT_SN_LOG_LEVEL);

BUILD_ASSERT(CONFIG_MQTT_SN_LIB_MAX_ADDR_SIZE >= sizeof(struct net_sockaddr_in) ||
	     !IS_ENABLED(CONFIG_NET_IPV4));

BUILD_ASSERT(CONFIG_MQTT_SN_LIB_MAX_ADDR_SIZE >= sizeof(struct net_sockaddr_in6) ||
	     !IS_ENABLED(CONFIG_NET_IPV6));

static char *get_ip_str(const struct net_sockaddr *sa, char *s, size_t maxlen)
{
	switch (sa->sa_family) {
	case NET_AF_INET:
		zsock_inet_ntop(NET_AF_INET, &(((struct net_sockaddr_in *)sa)->sin_addr),
				s, maxlen);
		break;

	case NET_AF_INET6:
		zsock_inet_ntop(NET_AF_INET6, &(((struct net_sockaddr_in6 *)sa)->sin6_addr),
				s, maxlen);
		break;

	default:
		strncpy(s, "Unknown AF", maxlen);
		break;
	}

	return s;
}

static int get_multicast_ttl(struct mqtt_sn_transport_udp *udp, int *ttl, net_socklen_t *ttl_len)
{
	if (udp->addr.sa_family == NET_AF_INET && IS_ENABLED(CONFIG_NET_IPV4)) {
		return zsock_getsockopt(udp->sock, NET_IPPROTO_IP, ZSOCK_IP_MULTICAST_TTL, ttl,
					ttl_len);
	} else if (udp->addr.sa_family == NET_AF_INET6 && IS_ENABLED(CONFIG_NET_IPV6)) {
		return zsock_getsockopt(udp->sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_MULTICAST_HOPS, ttl,
					ttl_len);
	}

	LOG_ERR("Unknown AF");
	return -EINVAL;
}

static int set_multicast_ttl(struct mqtt_sn_transport_udp *udp, int *ttl, net_socklen_t ttl_len)
{
	if (udp->addr.sa_family == NET_AF_INET && IS_ENABLED(CONFIG_NET_IPV4)) {
		return zsock_setsockopt(udp->sock, NET_IPPROTO_IP, ZSOCK_IP_MULTICAST_TTL, ttl,
					ttl_len);
	} else if (udp->addr.sa_family == NET_AF_INET6 && IS_ENABLED(CONFIG_NET_IPV6)) {
		return zsock_setsockopt(udp->sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_MULTICAST_HOPS, ttl,
					ttl_len);
	}

	LOG_ERR("Unknown AF");
	return -EINVAL;
}

static int tp_udp_init(struct mqtt_sn_transport *transport)
{
	struct mqtt_sn_transport_udp *udp = UDP_TRANSPORT(transport);
	int err;
	int errno_backup;
	struct net_sockaddr addrm;
	int optval;
	struct net_if *iface;

	if (udp->sock >= 0) {
		return -EALREADY;
	}
	if (!udp->use_broadcast) {
		return 0;
	}

	udp->sock = zsock_socket(udp->addr.sa_family, NET_SOCK_DGRAM, 0);
	if (udp->sock < 0) {
		return -errno;
	}

	LOG_DBG("Socket %d", udp->sock);

	optval = 1;
	err = zsock_setsockopt(udp->sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR,
			       &optval, sizeof(optval));
	if (err < 0) {
		return -errno;
	}

	if (IS_ENABLED(CONFIG_MQTT_SN_LOG_LEVEL_DBG)) {
		char ip[NET_INET6_ADDRSTRLEN], *out;
		uint16_t port = 0;

		out = get_ip_str((struct net_sockaddr *)&udp->addr, ip, sizeof(ip));
		switch (udp->addr.sa_family) {
		case NET_AF_INET:
			port = net_ntohs(((struct net_sockaddr_in *)&udp->addr)->sin_port);
			break;
		case NET_AF_INET6:
			port = net_ntohs(((struct net_sockaddr_in6 *)&udp->addr)->sin6_port);
			break;
		default:
			break;
		}

		if (out != NULL) {
			LOG_DBG("Binding to Broadcast IP %s:%u", out, port);
		}
	}

	switch (udp->addr.sa_family) {
	case NET_AF_INET:
		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			addrm.sa_family = NET_AF_INET;
			((struct net_sockaddr_in *)&addrm)->sin_port =
				((struct net_sockaddr_in *)&udp->addr)->sin_port;
			((struct net_sockaddr_in *)&addrm)->sin_addr.s_addr = NET_INADDR_ANY;
		}
		break;
	case NET_AF_INET6:
		if (IS_ENABLED(CONFIG_NET_IPV6)) {
			addrm.sa_family = NET_AF_INET6;
			((struct net_sockaddr_in6 *)&addrm)->sin6_port =
				((struct net_sockaddr_in6 *)&udp->addr)->sin6_port;
			memcpy(&((struct net_sockaddr_in6 *)&addrm)->sin6_addr, &net_in6addr_any,
			       sizeof(struct net_in6_addr));
			break;
		}
	default:
		LOG_ERR("Unknown AF");
		return -EINVAL;
	}

	err = zsock_bind(udp->sock, &addrm, sizeof(addrm));
	if (err) {
		errno_backup = errno;
		LOG_ERR("Error during bind: %d", errno_backup);
		return errno_backup;
	}

	if (udp->addr.sa_family == NET_AF_INET && IS_ENABLED(CONFIG_NET_IPV4)) {
		struct net_sockaddr_in *bcaddr_in = (struct net_sockaddr_in *)&udp->addr;
		struct net_ip_mreqn mreqn;

		iface = net_if_ipv4_select_src_iface(
			&((struct net_sockaddr_in *)&udp->addr)->sin_addr);

		mreqn = (struct net_ip_mreqn) {
			.imr_multiaddr = bcaddr_in->sin_addr,
			.imr_ifindex = net_if_get_by_iface(iface),
		};

		err = zsock_setsockopt(udp->sock, NET_IPPROTO_IP, ZSOCK_IP_ADD_MEMBERSHIP,
				       &mreqn, sizeof(mreqn));
		if (err < 0 && errno != EALREADY) {
			return -errno;
		}
	} else if (udp->addr.sa_family == NET_AF_INET6 && IS_ENABLED(CONFIG_NET_IPV6)) {
		struct net_sockaddr_in6 *bcaddr_in6 = (struct net_sockaddr_in6 *)&udp->addr;
		struct net_ipv6_mreq mreq;

		iface = net_if_ipv6_select_src_iface(
			&((struct net_sockaddr_in6 *)&addrm)->sin6_addr);

		mreq = (struct net_ipv6_mreq) {
			.ipv6mr_multiaddr = bcaddr_in6->sin6_addr,
			.ipv6mr_ifindex = net_if_get_by_iface(iface),
		};

		err = zsock_setsockopt(udp->sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_ADD_MEMBERSHIP,
				       &mreq, sizeof(mreq));
		if (err < 0 && errno != EALREADY) {
			return -errno;
		}
	} else {
		LOG_ERR("Unknown AF");
		return -EINVAL;
	}

	optval = CONFIG_MQTT_SN_LIB_BROADCAST_RADIUS;
	err = set_multicast_ttl(udp, &optval, sizeof(optval));
	if (err < 0) {
		return -errno;
	}

	return 0;
}

static void tp_udp_deinit(struct mqtt_sn_transport *transport)
{
	struct mqtt_sn_transport_udp *udp = UDP_TRANSPORT(transport);

	if (udp->sock >= 0) {
		zsock_close(udp->sock);
		udp->sock = -1;
	}
}

static int prepare_unicast_socket(struct mqtt_sn_transport_udp *udp,
				  const struct net_sockaddr *dest_addr, size_t addrlen)
{
	int ret;

	if (udp->use_broadcast) {
		return 0;
	}
	if (addrlen > sizeof(udp->addr)) {
		return -ENOMEM;
	}
	if (udp->sock >= 0 && sockaddr_equal(&udp->addr, dest_addr)) {
		return 0;
	}

	if (udp->sock >= 0) {
		zsock_close(udp->sock);
		udp->sock = -1;
	}

	udp->sock = zsock_socket(dest_addr->sa_family, NET_SOCK_DGRAM, 0);
	if (udp->sock < 0) {
		return -errno;
	}

	ret = zsock_connect(udp->sock, dest_addr, addrlen);
	if (ret < 0) {
		ret = -errno;
		zsock_close(udp->sock);
		udp->sock = -1;
		return ret;
	}

	memcpy(&udp->addr, dest_addr, addrlen);
	udp->addrlen = addrlen;

	return 0;
}

static int tp_udp_sendto(struct mqtt_sn_transport *transport, void *buf, size_t sz,
			 const void *dest_addr, size_t addrlen)
{
	struct mqtt_sn_transport_udp *udp = UDP_TRANSPORT(transport);
	int rc;
	int ttl;
	net_socklen_t ttl_len;

	if (dest_addr == NULL) {
		if (!udp->use_broadcast) {
			return -ENOTSUP;
		}

		LOG_HEXDUMP_DBG(buf, sz, "Sending Broadcast UDP packet");

		/* Set ttl if requested value does not match existing*/
		ttl_len = sizeof(ttl);
		rc = get_multicast_ttl(udp, &ttl, &ttl_len);
		if (rc < 0) {
			return -errno;
		}
		if (ttl != addrlen) {
			ttl = addrlen;
			rc = set_multicast_ttl(udp, &ttl, sizeof(ttl));
			if (rc < 0) {
				return -errno;
			}
		}

		rc = zsock_sendto(udp->sock, buf, sz, 0, &udp->addr, udp->addrlen);
	} else {
		rc = prepare_unicast_socket(udp, dest_addr, addrlen);
		if (rc < 0) {
			return rc;
		}

		LOG_HEXDUMP_DBG(buf, sz, "Sending Addressed UDP packet");
		rc = zsock_sendto(udp->sock, buf, sz, 0, dest_addr, addrlen);
	}

	if (rc < 0) {
		return -errno;
	}

	if (rc != sz) {
		return -EIO;
	}

	return 0;
}

static ssize_t tp_udp_recvfrom(struct mqtt_sn_transport *transport, void *buffer, size_t length,
			       void *src_addr, size_t *addrlen)
{
	struct mqtt_sn_transport_udp *udp = UDP_TRANSPORT(transport);
	ssize_t ret;
	int errno_backup;
	net_socklen_t addrlen_local = *addrlen;

	if (udp->sock < 0) {
		return 0;
	}

	ret = zsock_recvfrom(udp->sock, buffer, length, 0, src_addr, &addrlen_local);
	errno_backup = errno;
	LOG_DBG("recv %zd", ret);
	if (ret < 0) {
		errno = errno_backup;
		return -1;
	}
	*addrlen = addrlen_local;

	LOG_HEXDUMP_DBG(buffer, ret, "recv");

	return ret;
}

static int tp_udp_poll(struct mqtt_sn_transport *transport)
{
	struct mqtt_sn_transport_udp *udp = UDP_TRANSPORT(transport);
	int rc;

	struct zsock_pollfd pollfd = {
		.fd = udp->sock,
		.events = ZSOCK_POLLIN,
	};

	if (udp->sock < 0) {
		return 0;
	}

	rc = zsock_poll(&pollfd, 1, 0);
	if (rc < 1) {
		return rc;
	}

	LOG_DBG("revents %d", pollfd.revents & ZSOCK_POLLIN);

	return pollfd.revents & ZSOCK_POLLIN;
}

int mqtt_sn_transport_udp_init(struct mqtt_sn_transport_udp *udp, struct net_sockaddr *bcaddr,
			       net_socklen_t addrlen)
{
	if (!udp) {
		return -EINVAL;
	}

	memset(udp, 0, sizeof(*udp));

	udp->tp = (struct mqtt_sn_transport){.init = tp_udp_init,
					     .deinit = tp_udp_deinit,
					     .sendto = tp_udp_sendto,
					     .poll = tp_udp_poll,
					     .recvfrom = tp_udp_recvfrom};

	udp->sock = -1;

	if (bcaddr != NULL) {
		memcpy(&udp->addr, bcaddr, addrlen);
		udp->addrlen = addrlen;
		udp->use_broadcast = true;
	}

	return 0;
}
