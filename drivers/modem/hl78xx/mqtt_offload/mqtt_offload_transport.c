/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_transport.c
 *
 * @brief Internal functions to handle transport in MQTT module.
 */

#include "mqtt_offload_transport.h"

#define COMMON_TRANSPORT_PROCEDURE (MQTT_OFFLOAD_TRANSPORT_NUM - 1)
/**@brief Function pointer array for TCP/TLS transport handlers. */
const struct transport_procedure transport_fn[MQTT_OFFLOAD_TRANSPORT_NUM] = {
	{
		.connect = mqtt_offload_mqtt_connect,
	},
#if defined(CONFIG_MQTT_OFFLOAD_LIB_TLS)
	{
		.connect = mqtt_offload_tls_connect,
	},
#endif /* CONFIG_MQTT_OFFLOAD_LIB_TLS */
#if defined(CONFIG_MQTT_LIB_CUSTOM_TRANSPORT)
	{
		.connect = mqtt_client_custom_transport_connect,
	},
#endif /* CONFIG_MQTT_LIB_CUSTOM_TRANSPORT */
	{
		.close = mqtt_offload_close,
		.read = mqtt_offload_read,
		.write = mqtt_offload_write,
		.poll = mqtt_offload_poll
	},
};

int mqtt_transport_connect(struct mqtt_offload_client *client, const void *params)
{
	return transport_fn[client->transport->type].connect(client, params);
}

int mqtt_transport_write(struct mqtt_offload_client *client, void *buf, size_t sz,
			 const void *dest_addr, size_t addrlen)
{
	return transport_fn[COMMON_TRANSPORT_PROCEDURE].write(client, buf, sz, dest_addr, addrlen);
}

int mqtt_transport_read(struct mqtt_offload_client *client, void *buf, size_t sz,
			 void *src_addr, size_t *addrlen)
{
	return transport_fn[COMMON_TRANSPORT_PROCEDURE].read(client, buf, sz, src_addr, addrlen);
}

int mqtt_transport_close(struct mqtt_offload_client *client)
{
	return transport_fn[COMMON_TRANSPORT_PROCEDURE].close(client);
}

int mqtt_transport_poll(struct mqtt_offload_client *client)
{
	return transport_fn[COMMON_TRANSPORT_PROCEDURE].poll(client);
}
