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

#include <zephyr/posix/fcntl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_mqtt_sn, CONFIG_MQTT_SN_LOG_LEVEL);

static char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen)
{
	switch (sa->sa_family) {
	case AF_INET:
		inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
		break;

	case AF_INET6:
		inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
		break;

	default:
		strncpy(s, "Unknown AF", maxlen);
		break;
	}

	return s;
}

static int tp_udp_init(struct mqtt_sn_transport *transport)
{
	struct mqtt_sn_transport_udp *udp = UDP_TRANSPORT(transport);
	int err;
	struct sockaddr addrm;
	struct ip_mreqn mreqn;
	int optval;
	struct net_if *iface;

	udp->sock = zsock_socket(udp->bcaddr.sa_family, SOCK_DGRAM, 0);
	if (udp->sock < 0) {
		return errno;
	}

	LOG_DBG("Socket %d", udp->sock);

	optval = 1;
	err = zsock_setsockopt(udp->sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (err < 0) {
		return errno;
	}

	if (IS_ENABLED(CONFIG_MQTT_SN_LOG_LEVEL_DBG)) {
		char ip[30], *out;
		uint16_t port;

		out = get_ip_str((struct sockaddr *)&udp->bcaddr, ip, sizeof(ip));
		switch (udp->bcaddr.sa_family) {
		case AF_INET:
			port = ntohs(((struct sockaddr_in *)&udp->bcaddr)->sin_port);
			break;
		case AF_INET6:
			port = ntohs(((struct sockaddr_in6 *)&udp->bcaddr)->sin6_port);
			break;
		default:
			break;
		}

		if (out != NULL) {
			LOG_DBG("Binding to Brodcast IP %s:%u", out, port);
		}
	}

	switch (udp->bcaddr.sa_family) {
	case AF_INET:
		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			addrm.sa_family = AF_INET;
			((struct sockaddr_in *)&addrm)->sin_port =
				((struct sockaddr_in *)&udp->bcaddr)->sin_port;
			((struct sockaddr_in *)&addrm)->sin_addr.s_addr = INADDR_ANY;
		}
		break;
	case AF_INET6:
		if (IS_ENABLED(CONFIG_NET_IPV6)) {
			addrm.sa_family = AF_INET6;
			((struct sockaddr_in6 *)&addrm)->sin6_port =
				((struct sockaddr_in6 *)&udp->bcaddr)->sin6_port;
			memcpy(&((struct sockaddr_in6 *)&addrm)->sin6_addr, &in6addr_any,
			       sizeof(struct in6_addr));
			break;
		}
	default:
		LOG_ERR("Unknown AF");
		return -EINVAL;
	}

	err = zsock_bind(udp->sock, &addrm, sizeof(addrm));
	if (err) {
		LOG_ERR("Error during bind: %d", errno);
		return errno;
	}

	memcpy(&mreqn.imr_multiaddr, &udp->bcaddr.data[2], sizeof(udp->bcaddr.data) - 2);
	if (udp->bcaddr.sa_family == AF_INET && IS_ENABLED(CONFIG_NET_IPV4)) {
		iface = net_if_ipv4_select_src_iface(
			&((struct sockaddr_in *)&udp->bcaddr)->sin_addr);
	} else if (udp->bcaddr.sa_family == AF_INET6 && IS_ENABLED(CONFIG_NET_IPV6)) {
		iface = net_if_ipv6_select_src_iface(&((struct sockaddr_in6 *)&addrm)->sin6_addr);
	} else {
		LOG_ERR("Unknown AF");
		return -EINVAL;
	}
	mreqn.imr_ifindex = net_if_get_by_iface(iface);

	err = zsock_setsockopt(udp->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreqn, sizeof(mreqn));
	if (err < 0) {
		return errno;
	}

	optval = CONFIG_MQTT_SN_LIB_BROADCAST_RADIUS;
	err = zsock_setsockopt(udp->sock, IPPROTO_IP, IP_MULTICAST_TTL, &optval, sizeof(optval));
	if (err < 0) {
		return errno;
	}

	return 0;
}

static void tp_udp_deinit(struct mqtt_sn_transport *transport)
{
	struct mqtt_sn_transport_udp *udp = UDP_TRANSPORT(transport);

	zsock_close(udp->sock);
}

static int tp_udp_sendto(struct mqtt_sn_client *client, void *buf, size_t sz, const void *dest_addr,
			 size_t addrlen)
{
	struct mqtt_sn_transport_udp *udp = UDP_TRANSPORT(client->transport);
	int rc;
	int ttl;
	socklen_t ttl_len;

	if (dest_addr == NULL) {
		LOG_HEXDUMP_DBG(buf, sz, "Sending Broadcast UDP packet");

		/* Set ttl if requested value does not match existing*/
		rc = zsock_getsockopt(udp->sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, &ttl_len);
		if (rc < 0) {
			return -errno;
		}
		if (ttl != addrlen) {
			ttl = addrlen;
			rc = zsock_setsockopt(udp->sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
					      sizeof(ttl));
			if (rc < 0) {
				return -errno;
			}
		}

		rc = zsock_sendto(udp->sock, buf, sz, 0, &udp->bcaddr, udp->bcaddrlen);
	} else {
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

static ssize_t tp_udp_recvfrom(struct mqtt_sn_client *client, void *buffer, size_t length,
			       void *src_addr, size_t *addrlen)
{
	struct mqtt_sn_transport_udp *udp = UDP_TRANSPORT(client->transport);
	int rc;
	struct sockaddr *srcaddr = src_addr;

	rc = zsock_recvfrom(udp->sock, buffer, length, 0, src_addr, addrlen);
	LOG_DBG("recv %d", rc);
	if (rc < 0) {
		return -errno;
	}

	LOG_HEXDUMP_DBG(buffer, rc, "recv");

	if (*addrlen != udp->bcaddrlen) {
		return rc;
	}

	if (memcmp(srcaddr->data, udp->bcaddr.data, *addrlen) != 0) {
		return rc;
	}

	src_addr = NULL;
	*addrlen = 1;
	return rc;
}

static int tp_udp_poll(struct mqtt_sn_client *client)
{
	struct mqtt_sn_transport_udp *udp = UDP_TRANSPORT(client->transport);
	int rc;

	struct zsock_pollfd pollfd = {
		.fd = udp->sock,
		.events = ZSOCK_POLLIN,
	};

	rc = zsock_poll(&pollfd, 1, 0);
	if (rc < 1) {
		return rc;
	}

	LOG_DBG("revents %d", pollfd.revents & ZSOCK_POLLIN);

	return pollfd.revents & ZSOCK_POLLIN;
}

int mqtt_sn_transport_udp_init(struct mqtt_sn_transport_udp *udp, struct sockaddr *bcaddr,
			       socklen_t addrlen)
{
	if (!udp || !bcaddr || !addrlen) {
		return -EINVAL;
	}

	memset(udp, 0, sizeof(*udp));

	udp->tp = (struct mqtt_sn_transport){.init = tp_udp_init,
					     .deinit = tp_udp_deinit,
					     .sendto = tp_udp_sendto,
					     .poll = tp_udp_poll,
					     .recvfrom = tp_udp_recvfrom};

	udp->sock = 0;

	memcpy(&udp->bcaddr, bcaddr, addrlen);
	udp->bcaddrlen = addrlen;

	return 0;
}
