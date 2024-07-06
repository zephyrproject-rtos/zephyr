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

	udp->sock = zsock_socket(udp->gwaddr.sa_family, SOCK_DGRAM, 0);
	if (udp->sock < 0) {
		return errno;
	}

	LOG_DBG("Socket %d", udp->sock);

#ifdef LOG_DBG
	char ip[30], *out;

	out = get_ip_str((struct sockaddr *)&udp->gwaddr, ip, sizeof(ip));
	if (out != NULL) {
		LOG_DBG("Connecting to IP %s:%u", out,
			ntohs(((struct sockaddr_in *)&udp->gwaddr)->sin_port));
	}
#endif

	err = zsock_connect(udp->sock, (struct sockaddr *)&udp->gwaddr, udp->gwaddrlen);
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

static int tp_udp_msg_send(struct mqtt_sn_client *client, void *buf, size_t sz)
{
	struct mqtt_sn_transport_udp *udp = UDP_TRANSPORT(client->transport);
	int rc;

	LOG_HEXDUMP_DBG(buf, sz, "Sending UDP packet");

	rc = zsock_send(udp->sock, buf, sz, 0);
	if (rc < 0) {
		return -errno;
	}

	if (rc != sz) {
		return -EIO;
	}

	return 0;
}

static ssize_t tp_udp_recv(struct mqtt_sn_client *client, void *buffer, size_t length)
{
	struct mqtt_sn_transport_udp *udp = UDP_TRANSPORT(client->transport);
	int rc;

	rc = zsock_recv(udp->sock, buffer, length, 0);
	LOG_DBG("recv %d", rc);
	if (rc < 0) {
		return -errno;
	}

	LOG_HEXDUMP_DBG(buffer, rc, "recv");

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

int mqtt_sn_transport_udp_init(struct mqtt_sn_transport_udp *udp, struct sockaddr *gwaddr,
			       socklen_t addrlen)
{
	if (!udp || !gwaddr || !addrlen) {
		return -EINVAL;
	}

	memset(udp, 0, sizeof(*udp));

	udp->tp = (struct mqtt_sn_transport){.init = tp_udp_init,
					     .deinit = tp_udp_deinit,
					     .msg_send = tp_udp_msg_send,
					     .poll = tp_udp_poll,
					     .recv = tp_udp_recv};

	udp->sock = 0;

	memcpy(&udp->gwaddr, gwaddr, addrlen);
	udp->gwaddrlen = addrlen;

	return 0;
}
