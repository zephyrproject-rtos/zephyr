/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_offload_transport_mqtt.c
 *
 * @brief HL78XX-MQTT tls transport.
 */

#include <errno.h>
#include <errno.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

#include "mqtt_offload_msg.h"

#include <zephyr/net/mqtt_offload.h>
#include "mqtt_offload_transport.h"
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/igmp.h>
#include "mqtt_offload_socket.h"
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_mqtt_offload, CONFIG_MQTT_OFFLOAD_LOG_LEVEL);

int mqtt_offload_mqtt_connect(struct mqtt_offload_client *client, const void *params)
{
	struct mqtt_offload_transport_mqtt *mqtt = client->transport;
	const struct mqtt_offload_param *p = (const struct mqtt_offload_param *)params;
	const struct sockaddr *broker = client->broker;
	int err;

	mqtt->mqtt.sock = zsock_socket(broker->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (mqtt->mqtt.sock < 0) {
		return errno;
	}

	LOG_DBG("Socket %d", mqtt->mqtt.sock);

	if (p->params.connect.client_id.data != NULL && p->params.connect.client_id.size > 0) {
		err = zsock_setsockopt(mqtt->mqtt.sock, SOL_MQTT, MQTT_CLIENT_ID,
				       p->params.connect.client_id.data,
				       p->params.connect.client_id.size);
		if (err < 0) {
			NET_ERR("Failed to set client id error (%d)", -errno);
			goto error;
		}
	}

	err = zsock_setsockopt(mqtt->mqtt.sock, SOL_MQTT, MQTT_KEEP_ALIVE,
			       &p->params.connect.duration, sizeof(p->params.connect.duration));
	if (err < 0) {
		NET_ERR("Failed to set mqtt keep alive error (%d)", -errno);
		goto error;
	}

	err = zsock_setsockopt(mqtt->mqtt.sock, SOL_MQTT, MQTT_CLEAN_SESSION,
			       &p->params.connect.clean_session,
			       sizeof(p->params.connect.clean_session));
	if (err < 0) {
		NET_ERR("Failed to set mqtt clean session error (%d)", -errno);
		goto error;
	}

	err = zsock_setsockopt(mqtt->mqtt.sock, SOL_MQTT, MQTT_QOS, &p->params.connect.qos,
			       sizeof(p->params.connect.qos));
	if (err < 0) {
		NET_ERR("Failed to set mqtt qos error (%d)", -errno);
		goto error;
	}

	size_t peer_addr_size = sizeof(struct sockaddr_in6);

	if (broker->sa_family == AF_INET) {
		peer_addr_size = sizeof(struct sockaddr_in);
	}

	err = zsock_connect(mqtt->mqtt.sock, client->broker, peer_addr_size);
	if (err < 0) {
		goto error;
	}

	NET_DBG("Connect completed");
	return 0;
error:
	return err;
}

int mqtt_offload_close(struct mqtt_offload_client *client)
{
	struct mqtt_offload_transport_mqtt *mqtt = client->transport;

	return zsock_close(mqtt->mqtt.sock);
}

int mqtt_offload_write(struct mqtt_offload_client *client, void *buf, size_t sz,
		       const void *dest_addr, size_t addrlen)
{
	struct mqtt_offload_transport_mqtt *mqtt = client->transport;
	int rc;

	if (dest_addr == NULL) {
		LOG_HEXDUMP_DBG(buf, sz, "Sending Broadcast MQTT packet");

		/* Set ttl if requested value does not match existing*/

		rc = zsock_sendto(mqtt->mqtt.sock, buf, sz, 0, client->broker,
				  sizeof(client->broker));
	} else {
		LOG_HEXDUMP_DBG(buf, sz, "Sending Addressed MQTT packet");
		rc = zsock_sendto(mqtt->mqtt.sock, buf, sz, 0, dest_addr, addrlen);
	}

	if (rc < 0) {
		return -errno;
	}

	if (rc != sz) {
		return -EIO;
	}

	return 0;
}

ssize_t mqtt_offload_read(struct mqtt_offload_client *client, void *rx_buf, size_t rx_len,
			  void *src_addr, size_t *addrlen)
{
	struct mqtt_offload_transport_mqtt *mqtt = client->transport;
	int rc;

	rc = zsock_recvfrom(mqtt->mqtt.sock, rx_buf, rx_len, 0, src_addr, addrlen);
	if (rc < 0) {
		return -errno;
	}

	LOG_HEXDUMP_DBG(rx_buf, rc, "recv");

	if (*addrlen != sizeof(client->broker)) {
		return rc;
	}

	src_addr = NULL;
	*addrlen = 1;
	return rc;
}

int mqtt_offload_poll(struct mqtt_offload_client *client)
{
	struct mqtt_offload_transport_mqtt *mqtt = client->transport;
	int rc;

	struct zsock_pollfd pollfd = {
		.fd = mqtt->mqtt.sock,
		.events = ZSOCK_POLLIN,
	};

	rc = zsock_poll(&pollfd, 1, 0);
	if (rc < 1) {
		return rc;
	}

	LOG_DBG("revents %d", pollfd.revents & ZSOCK_POLLIN);

	return pollfd.revents & ZSOCK_POLLIN;
}
