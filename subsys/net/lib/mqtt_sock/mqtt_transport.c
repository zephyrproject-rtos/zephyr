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

/* Transport handler functions for TCP socket transport. */
extern int mqtt_client_tcp_connect(struct mqtt_client *client);
extern int mqtt_client_tcp_write(struct mqtt_client *client, const u8_t *data,
				 u32_t datalen);
extern int mqtt_client_tcp_read(struct mqtt_client *client, u8_t *data,
				u32_t buflen);
extern int mqtt_client_tcp_disconnect(struct mqtt_client *client);

/**@brief Function pointer array for TCP/TLS transport handlers. */
const struct transport_procedure transport_fn[MQTT_TRANSPORT_NUM] = {
	{
		mqtt_client_tcp_connect,
		mqtt_client_tcp_write,
		mqtt_client_tcp_read,
		mqtt_client_tcp_disconnect,
	}
};

int mqtt_transport_connect(struct mqtt_client *client)
{
	return transport_fn[client->transport.type].connect(client);
}

int mqtt_transport_write(struct mqtt_client *client, const u8_t *data,
			 u32_t datalen)
{
	return transport_fn[client->transport.type].write(client, data,
							  datalen);
}

int mqtt_transport_read(struct mqtt_client *client, u8_t *data, u32_t buflen)
{
	return transport_fn[client->transport.type].read(client, data, buflen);
}

int mqtt_transport_disconnect(struct mqtt_client *client)
{
	return transport_fn[client->transport.type].disconnect(client);
}
