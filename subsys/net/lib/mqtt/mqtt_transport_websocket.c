/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_transport_websocket.c
 *
 * @brief Internal functions to handle transport over Websocket.
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_websocket, CONFIG_MQTT_LOG_LEVEL);

#include <errno.h>
#include <net/socket.h>
#include <net/mqtt.h>
#include <net/websocket.h>

#include "mqtt_os.h"
#include "mqtt_transport.h"

int mqtt_client_websocket_connect(struct mqtt_client *client)
{
	const char *extra_headers[] = {
		"Sec-WebSocket-Protocol: mqtt\r\n",
		NULL
	};
	int transport_sock;
	int ret;

	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE_WEBSOCKET) {
		ret = mqtt_client_tcp_connect(client);
		if (ret < 0) {
			return ret;
		}

		transport_sock = client->transport.tcp.sock;
	}
#if defined(CONFIG_MQTT_LIB_TLS)
	else if (client->transport.type == MQTT_TRANSPORT_SECURE_WEBSOCKET) {
		ret = mqtt_client_tls_connect(client);
		if (ret < 0) {
			return ret;
		}

		transport_sock = client->transport.tls.sock;
	}
#endif
	else {
		return -EINVAL;
	}

	if (client->transport.websocket.config.url == NULL) {
		client->transport.websocket.config.url = "/mqtt";
	}

	if (client->transport.websocket.config.host == NULL) {
		client->transport.websocket.config.host = "localhost";
	}

	/* If application needs to set some extra header options, then
	 * it can set the optional_headers_cb. In this case the app
	 * will need to also send "Sec-WebSocket-Protocol: mqtt\r\n"
	 * field as the optional_headers field is ignored if the callback
	 * is set.
	 */
	client->transport.websocket.config.optional_headers = extra_headers;

	client->transport.websocket.sock = websocket_connect(
			transport_sock,
			&client->transport.websocket.config,
			client->transport.websocket.timeout,
			NULL);
	if (client->transport.websocket.sock < 0) {
		MQTT_TRC("Websocket connect failed (%d)",
			 client->transport.websocket.sock);

		(void)close(transport_sock);
		return client->transport.websocket.sock;
	}

	MQTT_TRC("Connect completed");

	return 0;
}

int mqtt_client_websocket_write(struct mqtt_client *client, const u8_t *data,
				u32_t datalen)
{
	u32_t offset = 0U;
	int ret;

	while (offset < datalen) {
		ret = websocket_send_msg(client->transport.websocket.sock,
					 data + offset, datalen - offset,
					 WEBSOCKET_OPCODE_DATA_BINARY,
					 true, true, K_FOREVER);
		if (ret < 0) {
			return -errno;
		}

		offset += ret;
	}

	return 0;
}

int mqtt_client_websocket_read(struct mqtt_client *client, u8_t *data,
			       u32_t buflen, bool shall_block)
{
	s32_t timeout = K_FOREVER;

	if (!shall_block) {
		timeout = K_NO_WAIT;
	}

	return websocket_recv_msg(client->transport.websocket.sock,
				  data, buflen, NULL, NULL, timeout);
}

int mqtt_client_websocket_disconnect(struct mqtt_client *client)
{
	MQTT_TRC("Closing socket %d", client->transport.websocket.sock);

	return websocket_disconnect(client->transport.websocket.sock);
}
