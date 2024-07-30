/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_transport.c
 *
 * @brief Internal functions to handle transport in MQTT module.
 */

#include "mqtt_transport.h"

/**@brief Function pointer array for TCP/TLS transport handlers. */
const struct transport_procedure transport_fn[MQTT_TRANSPORT_NUM] = {
	{
		mqtt_client_tcp_connect,
		mqtt_client_tcp_write,
		mqtt_client_tcp_write_msg,
		mqtt_client_tcp_read,
		mqtt_client_tcp_disconnect,
	},
#if defined(CONFIG_MQTT_LIB_TLS)
	{
		mqtt_client_tls_connect,
		mqtt_client_tls_write,
		mqtt_client_tls_write_msg,
		mqtt_client_tls_read,
		mqtt_client_tls_disconnect,
	},
#endif /* CONFIG_MQTT_LIB_TLS */
#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
	{
		mqtt_client_websocket_connect,
		mqtt_client_websocket_write,
		mqtt_client_websocket_write_msg,
		mqtt_client_websocket_read,
		mqtt_client_websocket_disconnect,
	},
#if defined(CONFIG_MQTT_LIB_TLS)
	{
		mqtt_client_websocket_connect,
		mqtt_client_websocket_write,
		mqtt_client_websocket_write_msg,
		mqtt_client_websocket_read,
		mqtt_client_websocket_disconnect,
	},
#endif /* CONFIG_MQTT_LIB_TLS */
#endif /* CONFIG_MQTT_LIB_WEBSOCKET */
#if defined(CONFIG_MQTT_LIB_CUSTOM_TRANSPORT)
	{
		mqtt_client_custom_transport_connect,
		mqtt_client_custom_transport_write,
		mqtt_client_custom_transport_write_msg,
		mqtt_client_custom_transport_read,
		mqtt_client_custom_transport_disconnect,
	},
#endif /* CONFIG_MQTT_LIB_CUSTOM_TRANSPORT */
};

int mqtt_transport_connect(struct mqtt_client *client)
{
	return transport_fn[client->transport.type].connect(client);
}

int mqtt_transport_write(struct mqtt_client *client, const uint8_t *data,
			 uint32_t datalen)
{
	return transport_fn[client->transport.type].write(client, data,
							  datalen);
}

int mqtt_transport_write_msg(struct mqtt_client *client,
			     const struct msghdr *message)
{
	return transport_fn[client->transport.type].write_msg(client, message);
}

int mqtt_transport_read(struct mqtt_client *client, uint8_t *data, uint32_t buflen,
			bool shall_block)
{
	return transport_fn[client->transport.type].read(client, data, buflen,
							 shall_block);
}

int mqtt_transport_disconnect(struct mqtt_client *client)
{
	return transport_fn[client->transport.type].disconnect(client);
}
