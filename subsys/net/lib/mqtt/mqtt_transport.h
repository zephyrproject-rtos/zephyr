/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_transport.h
 *
 * @brief Internal functions to handle transport in MQTT module.
 */

#ifndef MQTT_TRANSPORT_H_
#define MQTT_TRANSPORT_H_

#include <zephyr/net/mqtt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Transport for handling transport connect procedure. */
typedef int (*transport_connect_handler_t)(struct mqtt_client *client);

/**@brief Transport write handler. */
typedef int (*transport_write_handler_t)(struct mqtt_client *client,
					 const uint8_t *data, uint32_t datalen);

/**@brief Transport write message handler, similar to POSIX sendmsg function. */
typedef int (*transport_write_msg_handler_t)(struct mqtt_client *client,
					     const struct msghdr *message);

/**@brief Transport read handler. */
typedef int (*transport_read_handler_t)(struct mqtt_client *client, uint8_t *data,
					uint32_t buflen, bool shall_block);

/**@brief Transport disconnect handler. */
typedef int (*transport_disconnect_handler_t)(struct mqtt_client *client);

/**@brief Transport procedure handlers. */
struct transport_procedure {
	/** Transport connect handler. Handles TCP connection callback based on
	 *  type of transport.
	 */
	transport_connect_handler_t connect;

	/** Transport write handler. Handles transport write based on type of
	 *  transport.
	 */
	transport_write_handler_t write;

	/** Transport write message handler. Handles transport write based
	 *  on type of transport.
	 */
	transport_write_msg_handler_t write_msg;

	/** Transport read handler. Handles transport read based on type of
	 *  transport.
	 */
	transport_read_handler_t read;

	/** Transport disconnect handler. Handles transport disconnection based
	 *  on type of transport.
	 */
	transport_disconnect_handler_t disconnect;
};

/**@brief Handles TCP Connection Complete for configured transport.
 *
 * @param[in] client Identifies the client on which the procedure is requested.
 *
 * @retval 0 or an error code indicating reason for failure.
 */
int mqtt_transport_connect(struct mqtt_client *client);

/**@brief Handles write requests on configured transport.
 *
 * @param[in] client Identifies the client on which the procedure is requested.
 * @param[in] data Data to be written on the transport.
 * @param[in] datalen Length of data to be written on the transport.
 *
 * @retval 0 or an error code indicating reason for failure.
 */
int mqtt_transport_write(struct mqtt_client *client, const uint8_t *data,
			 uint32_t datalen);

/**@brief Handles write message requests on configured transport.
 *
 * @param[in] client Identifies the client on which the procedure is requested.
 * @param[in] message Pointer to the `struct msghdr` structure, containing data
 *            to be written on the transport.
 *
 * @retval 0 or an error code indicating reason for failure.
 */
int mqtt_transport_write_msg(struct mqtt_client *client,
			     const struct msghdr *message);

/**@brief Handles read requests on configured transport.
 *
 * @param[in] client Identifies the client on which the procedure is requested.
 * @param[in] data Pointer where read data is to be fetched.
 * @param[in] buflen Size of memory provided for the operation.
 * @param[in] shall_block Information whether the call should block or not.
 *
 * @retval Number of bytes read or an error code indicating reason for failure.
 *         0 if connection was closed.
 */
int mqtt_transport_read(struct mqtt_client *client, uint8_t *data, uint32_t buflen,
			bool shall_block);

/**@brief Handles transport disconnection requests on configured transport.
 *
 * @param[in] client Identifies the client on which the procedure is requested.
 *
 * @retval 0 or an error code indicating reason for failure.
 */
int mqtt_transport_disconnect(struct mqtt_client *client);

/* Transport handler functions for TCP socket transport. */
int mqtt_client_tcp_connect(struct mqtt_client *client);
int mqtt_client_tcp_write(struct mqtt_client *client, const uint8_t *data,
			  uint32_t datalen);
int mqtt_client_tcp_write_msg(struct mqtt_client *client,
			      const struct msghdr *message);
int mqtt_client_tcp_read(struct mqtt_client *client, uint8_t *data,
			 uint32_t buflen, bool shall_block);
int mqtt_client_tcp_disconnect(struct mqtt_client *client);

#if defined(CONFIG_MQTT_LIB_TLS)
/* Transport handler functions for TLS socket transport. */
int mqtt_client_tls_connect(struct mqtt_client *client);
int mqtt_client_tls_write(struct mqtt_client *client, const uint8_t *data,
			  uint32_t datalen);
int mqtt_client_tls_write_msg(struct mqtt_client *client,
			      const struct msghdr *message);
int mqtt_client_tls_read(struct mqtt_client *client, uint8_t *data,
			 uint32_t buflen, bool shall_block);
int mqtt_client_tls_disconnect(struct mqtt_client *client);
#endif /* CONFIG_MQTT_LIB_TLS */

#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
int mqtt_client_websocket_connect(struct mqtt_client *client);
int mqtt_client_websocket_write(struct mqtt_client *client, const uint8_t *data,
				uint32_t datalen);
int mqtt_client_websocket_write_msg(struct mqtt_client *client,
				    const struct msghdr *message);
int mqtt_client_websocket_read(struct mqtt_client *client, uint8_t *data,
			       uint32_t buflen, bool shall_block);
int mqtt_client_websocket_disconnect(struct mqtt_client *client);
#endif

#if defined(CONFIG_MQTT_LIB_CUSTOM_TRANSPORT)
int mqtt_client_custom_transport_connect(struct mqtt_client *client);
int mqtt_client_custom_transport_write(struct mqtt_client *client, const uint8_t *data,
				uint32_t datalen);
int mqtt_client_custom_transport_write_msg(struct mqtt_client *client,
				    const struct msghdr *message);
int mqtt_client_custom_transport_read(struct mqtt_client *client, uint8_t *data,
			       uint32_t buflen, bool shall_block);
int mqtt_client_custom_transport_disconnect(struct mqtt_client *client);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MQTT_TRANSPORT_H_ */
