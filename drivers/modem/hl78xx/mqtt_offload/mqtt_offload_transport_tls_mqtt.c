/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_offload_transport_mqtt.c
 *
 * @brief MQTT-OFFLOAD tls transport.
 */

#include <errno.h>
#include <errno.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

#include "mqtt_offload_msg.h"
#include "mqtt_offload_socket.h"

#include <zephyr/net/mqtt_offload.h>
#include "mqtt_offload_transport.h"
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/igmp.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_mqtt_offload, CONFIG_MQTT_OFFLOAD_LOG_LEVEL);

static int set_sockopt_checked(int sock, int level, int optname, const void *optval,
			       socklen_t optlen, const char *errmsg)
{
	int err = zsock_setsockopt(sock, level, optname, optval, optlen);

	if (err < 0) {
		NET_ERR("%s (%d)", errmsg, -errno);
	}
	return err;
}

static int configure_tls_options(struct mqtt_offload_transport_mqtt *mqtt)
{
	int err;

	if (mqtt->mqtt.config.cipher_list && mqtt->mqtt.config.cipher_count > 0) {
		err = set_sockopt_checked(mqtt->mqtt.sock, SOL_MQTT, TLS_CIPHERSUITE_LIST,
					  mqtt->mqtt.config.cipher_list,
					  sizeof(int) * mqtt->mqtt.config.cipher_count,
					  "Failed to set TLS ciphersuite list");
		if (err < 0) {
			return err;
		}
	}

	if (mqtt->mqtt.config.sec_tag_list && mqtt->mqtt.config.sec_tag_count > 0) {
		err = set_sockopt_checked(mqtt->mqtt.sock, SOL_MQTT, TLS_SEC_TAG_LIST,
					  mqtt->mqtt.config.sec_tag_list,
					  sizeof(sec_tag_t) * mqtt->mqtt.config.sec_tag_count,
					  "Failed to set TLS sec tag list");
		if (err < 0) {
			return err;
		}
	}

	if (mqtt->mqtt.config.hostname) {
		err = set_sockopt_checked(
			mqtt->mqtt.sock, SOL_MQTT, TLS_HOSTNAME, mqtt->mqtt.config.hostname,
			strlen(mqtt->mqtt.config.hostname) + 1, "Failed to set TLS hostname");
		if (err < 0) {
			return err;
		}
	}

	if (mqtt->mqtt.config.cert_nocopy != TLS_CERT_NOCOPY_NONE) {
		err = set_sockopt_checked(
			mqtt->mqtt.sock, SOL_MQTT, TLS_CERT_NOCOPY, &mqtt->mqtt.config.cert_nocopy,
			sizeof(mqtt->mqtt.config.cert_nocopy), "Failed to set TLS cert nocopy");
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

static int configure_mqtt_options(struct mqtt_offload_transport_mqtt *mqtt,
				  const struct mqtt_offload_param *p)
{
	int err;

	if (p->params.connect.client_id.data && p->params.connect.client_id.size > 0) {
		err = set_sockopt_checked(
			mqtt->mqtt.sock, SOL_MQTT, MQTT_CLIENT_ID, p->params.connect.client_id.data,
			p->params.connect.client_id.size, "Failed to set client id");
		if (err < 0) {
			return err;
		}
	}

	err = set_sockopt_checked(mqtt->mqtt.sock, SOL_MQTT, MQTT_KEEP_ALIVE,
				  &p->params.connect.duration, sizeof(p->params.connect.duration),
				  "Failed to set mqtt keep alive");
	if (err < 0) {
		return err;
	}

	err = set_sockopt_checked(
		mqtt->mqtt.sock, SOL_MQTT, MQTT_CLEAN_SESSION, &p->params.connect.clean_session,
		sizeof(p->params.connect.clean_session), "Failed to set mqtt clean session");
	if (err < 0) {
		return err;
	}

	err = set_sockopt_checked(mqtt->mqtt.sock, SOL_MQTT, MQTT_QOS, &p->params.connect.qos,
				  sizeof(p->params.connect.qos), "Failed to set mqtt qos");
	if (err < 0) {
		return err;
	}

	return 0;
}

int mqtt_offload_tls_connect(struct mqtt_offload_client *client, const void *params)
{
	struct mqtt_offload_transport_mqtt *mqtt = client->transport;
	const struct mqtt_offload_param *p = (const struct mqtt_offload_param *)params;
	const struct sockaddr *broker = client->broker;
	int err;

	LOG_DBG("%d", __LINE__);

	mqtt->mqtt.sock = zsock_socket(broker->sa_family, SOCK_STREAM, IPPROTO_TLS_1_2);
	if (mqtt->mqtt.sock < 0) {
		return -errno;
	}

	LOG_DBG("Socket %d", mqtt->mqtt.sock);

	err = configure_tls_options(mqtt);
	if (err < 0) {
		goto error;
	}

	err = configure_mqtt_options(mqtt, p);
	if (err < 0) {
		goto error;
	}

	size_t peer_addr_size = (broker->sa_family == AF_INET) ? sizeof(struct sockaddr_in)
							       : sizeof(struct sockaddr_in6);

	err = zsock_connect(mqtt->mqtt.sock, client->broker, peer_addr_size);
	if (err < 0) {
		goto error;
	}

	NET_DBG("Connect completed");
	return 0;

error:
	return err;
}
