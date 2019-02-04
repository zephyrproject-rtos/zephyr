/*
 * Copyright (c) 2019 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_transport_socks.c
 *
 * @brief Internal functions to handle transport over SOCKS5 proxy.
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_socks, CONFIG_MQTT_LOG_LEVEL);

#include <errno.h>
#include <net/socket.h>
#include <net/socks.h>
#include <net/mqtt.h>

#include "mqtt_os.h"

/**@brief Handles connect request for TCP socket transport.
 *
 * @param[in] client Identifies the client on which the procedure is requested.
 *
 * @retval 0 or an error code indicating reason for failure.
 */
int mqtt_client_socks5_connect(struct mqtt_client *client)
{
	const struct sockaddr *broker = client->broker;
	const struct sockaddr *proxy =
		(struct sockaddr *)client->transport.socks5.proxy;

	if (proxy == NULL || broker == NULL) {
		return -EINVAL;
	}

	client->transport.socks5.sock =
		socks5_client_tcp_connect(proxy, broker);

	if (client->transport.socks5.sock < 0) {
		return client->transport.socks5.sock;
	}

	MQTT_TRC("Connect completed");
	return 0;
}
