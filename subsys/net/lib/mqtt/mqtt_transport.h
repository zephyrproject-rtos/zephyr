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

#include <net/mqtt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Transport for handling transport connect procedure. */
typedef int (*transport_connect_handler_t)(struct mqtt_client *client);

/**@brief Transport write handler. */
typedef int (*transport_write_handler_t)(struct mqtt_client *client,
					 const u8_t *data, u32_t datalen);

/**@brief Transport read handler. */
typedef int (*transport_read_handler_t)(struct mqtt_client *client, u8_t *data,
					u32_t buflen);

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
int mqtt_transport_write(struct mqtt_client *client, const u8_t *data,
			 u32_t datalen);

/**@brief Handles read requests on configured transport.
 *
 * @param[in] client Identifies the client on which the procedure is requested.
 * @param[in] data Pointer where read data is to be fetched.
 * @param[in] buflen Size of memory provided for the operation.
 *
 * @retval Number of bytes read or an error code indicating reason for failure.
 *         0 if connection was closed.
 */
int mqtt_transport_read(struct mqtt_client *client, u8_t *data, u32_t buflen);

/**@brief Handles transport disconnection requests on configured transport.
 *
 * @param[in] client Identifies the client on which the procedure is requested.
 *
 * @retval 0 or an error code indicating reason for failure.
 */
int mqtt_transport_disconnect(struct mqtt_client *client);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_TRANSPORT_H_ */
