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

#if defined(CONFIG_MQTT_LIB_TLS)
/* Transport handler functions for TLS socket transport. */
extern int mqtt_client_tls_connect(struct mqtt_client *client);
extern int mqtt_client_tls_write(struct mqtt_client *client, const u8_t *data,
				 u32_t datalen);
extern int mqtt_client_tls_read(struct mqtt_client *client, u8_t *data,
				u32_t buflen);
extern int mqtt_client_tls_disconnect(struct mqtt_client *client);
#endif /* CONFIG_MQTT_LIB_TLS */

#if defined(CONFIG_MQTT_LIB_SOCKS)
/* Transport handler functions for SOCKS5 proxy socket transport. */
extern int mqtt_client_socks5_connect(struct mqtt_client *client);
#endif /* CONFIG_MQTT_LIB_SOCKS */

/**@brief Function pointer array for TCP/TLS transport handlers. */
const struct transport_procedure transport_fn[MQTT_TRANSPORT_NUM] = {
	{
		mqtt_client_tcp_connect,
		mqtt_client_tcp_write,
		mqtt_client_tcp_read,
		mqtt_client_tcp_disconnect,
	},
#if defined(CONFIG_MQTT_LIB_TLS)
	{
		mqtt_client_tls_connect,
		mqtt_client_tls_write,
		mqtt_client_tls_read,
		mqtt_client_tls_disconnect,
	},
#endif /* CONFIG_MQTT_LIB_TLS */
#if defined(CONFIG_MQTT_LIB_SOCKS)
	{
		mqtt_client_socks5_connect,
		mqtt_client_tcp_write,
		mqtt_client_tcp_read,
		mqtt_client_tcp_disconnect,
	},
#endif /* CONFIG_MQTT_LIB_SOCKS */
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
