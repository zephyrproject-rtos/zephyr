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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_websocket, CONFIG_MQTT_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/websocket.h>

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
		NET_ERR("Websocket connect failed (%d)",
			 client->transport.websocket.sock);

		(void)close(transport_sock);
		return client->transport.websocket.sock;
	}

	NET_DBG("Connect completed");

	return 0;
}

int mqtt_client_websocket_write(struct mqtt_client *client, const uint8_t *data,
				uint32_t datalen)
{
	uint32_t offset = 0U;
	int ret;

	while (offset < datalen) {
		ret = websocket_send_msg(client->transport.websocket.sock,
					 data + offset, datalen - offset,
					 WEBSOCKET_OPCODE_DATA_BINARY,
					 true, true, SYS_FOREVER_MS);
		if (ret < 0) {
			return -errno;
		}

		offset += ret;
	}

	return 0;
}

int mqtt_client_websocket_write_msg(struct mqtt_client *client,
				    const struct msghdr *message)
{
	enum websocket_opcode opcode = WEBSOCKET_OPCODE_DATA_BINARY;
	bool final = false;
	ssize_t len;
	ssize_t ret;
	int i;

	len = 0;
	for (i = 0; i < message->msg_iovlen; i++) {
		if (i == message->msg_iovlen - 1) {
			final = true;
		}

		ret = websocket_send_msg(client->transport.websocket.sock,
					 message->msg_iov[i].iov_base,
					 message->msg_iov[i].iov_len, opcode,
					 true, final, SYS_FOREVER_MS);
		if (ret < 0) {
			return ret;
		}

		opcode = WEBSOCKET_OPCODE_CONTINUE;
		len += ret;
	}

	return len;
}

int mqtt_client_websocket_read(struct mqtt_client *client, uint8_t *data,
			       uint32_t buflen, bool shall_block)
{
	int32_t timeout = SYS_FOREVER_MS;
	uint32_t message_type = 0U;
	int ret;

	if (!shall_block) {
		timeout = 0;
	}

	ret = websocket_recv_msg(client->transport.websocket.sock,
				 data, buflen, &message_type, NULL, timeout);
	if (ret > 0 && message_type > 0) {
		if (message_type & WEBSOCKET_FLAG_CLOSE) {
			return 0;
		}

		if (!(message_type & WEBSOCKET_FLAG_BINARY)) {
			return -EAGAIN;
		}
	}

	return ret;
}

int mqtt_client_websocket_disconnect(struct mqtt_client *client)
{
	NET_INFO("Closing socket %d", client->transport.websocket.sock);

	return websocket_disconnect(client->transport.websocket.sock);
}
