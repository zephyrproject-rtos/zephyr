/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_transport.h
 *
 * @brief Internal functions to handle transport in MQTT module.
 */

#ifndef MQTT_OFFLOAD_TRANSPORT_H_
#define MQTT_OFFLOAD_TRANSPORT_H_
#include <zephyr/types.h>
#include <sys/types.h>

#include <zephyr/net/mqtt_offload.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Callback for initiating a transport connection.
 *
 *  @param client Pointer to the MQTT offload client instance.
 *  @param params Transport-specific connection parameters.
 *  @return 0 on success, or a negative error code on failure.
 */
typedef int (*transport_connect_handler_t)(struct mqtt_offload_client *client, const void *params);

/** @brief Callback for writing data to the transport.
 *
 *  @param client Pointer to the MQTT offload client instance.
 *  @param buf Buffer containing data to be sent.
 *  @param sz Size of the data buffer.
 *  @param dest_addr Destination address (transport-specific).
 *  @param addrlen Length of the destination address.
 *  @return 0 on success, or a negative error code on failure.
 */
typedef int (*transport_write_handler_t)(struct mqtt_offload_client *client, void *buf, size_t sz,
					 const void *dest_addr, size_t addrlen);

/** @brief Callback for sending a message using transport, similar to POSIX `sendmsg`.
 *
 *  @param client Pointer to the MQTT client instance.
 *  @param message Pointer to a `msghdr` structure containing message data.
 *  @return 0 on success, or a negative error code on failure.
 */
typedef int (*transport_write_msg_handler_t)(struct mqtt_client *client,
					     const struct msghdr *message);

/** @brief Callback for reading data from the transport.
 *
 *  @param client Pointer to the MQTT offload client instance.
 *  @param rx_buf Buffer to store received data.
 *  @param rx_len Maximum number of bytes to read.
 *  @param src_addr Pointer to store source address (transport-specific).
 *  @param addrlen Pointer to store length of source address.
 *  @return Number of bytes read, 0 if connection closed, or a negative error code.
 */
typedef ssize_t (*transport_read_handler_t)(struct mqtt_offload_client *client, void *rx_buf,
					    size_t rx_len, void *src_addr, size_t *addrlen);

/** @brief Callback for disconnecting the transport.
 *
 *  @param client Pointer to the MQTT offload client instance.
 *  @return 0 on success, or a negative error code on failure.
 */
typedef int (*transport_disconnect_handler_t)(struct mqtt_offload_client *client);

/** @brief Callback to check for available incoming data.
 *
 *  @param client Pointer to the MQTT offload client instance.
 *  @return 0 if no data is available, positive value if data is available, or a negative error
 * code.
 */
typedef int (*transport_poll_handler_t)(struct mqtt_offload_client *client);

/** @brief Structure holding transport operation callbacks.
 *
 *  Provides function pointers for handling various transport operations
 *  such as connect, read, write, poll, and disconnect.
 */
struct transport_procedure {
	transport_connect_handler_t connect;     /**< Connect operation handler */
	transport_write_handler_t write;         /**< Write operation handler */
	transport_write_msg_handler_t write_msg; /**< Write message handler */
	transport_read_handler_t read;           /**< Read operation handler */
	transport_disconnect_handler_t close;    /**< Disconnect operation handler */
	transport_poll_handler_t poll;           /**< Poll operation handler */
};

/** @brief Initiates a connection for the MQTT client.
 *
 *  @param client Pointer to the MQTT offload client.
 *  @param params Transport-specific connection parameters.
 *  @return 0 on success, or a negative error code.
 */
int mqtt_transport_connect(struct mqtt_offload_client *client, const void *params);

/** @brief Writes data to the transport.
 *
 *  @param client Pointer to the MQTT offload client.
 *  @param buf Data buffer to send.
 *  @param sz Size of the data buffer.
 *  @param dest_addr Destination address.
 *  @param addrlen Length of the destination address.
 *  @return 0 on success, or a negative error code.
 */
int mqtt_transport_write(struct mqtt_offload_client *client, void *buf, size_t sz,
			 const void *dest_addr, size_t addrlen);

/** @brief Sends a message using the transport.
 *
 *  @param client Pointer to the MQTT client.
 *  @param message Pointer to the message header structure.
 *  @return 0 on success, or a negative error code.
 */
int mqtt_transport_write_msg(struct mqtt_client *client, const struct msghdr *message);

/** @brief Reads data from the transport.
 *
 *  @param client Pointer to the MQTT offload client.
 *  @param buf Buffer to store received data.
 *  @param sz Maximum number of bytes to read.
 *  @param src_addr Pointer to store source address.
 *  @param addrlen Pointer to store length of source address.
 *  @return Number of bytes read, 0 if connection closed, or a negative error code.
 */
int mqtt_transport_read(struct mqtt_offload_client *client, void *buf, size_t sz, void *src_addr,
			size_t *addrlen);

/** @brief Checks for incoming data availability.
 *
 *  @param client Pointer to the MQTT offload client.
 *  @return 0 if no data, positive value if data is available, or a negative error code.
 */
int mqtt_transport_poll(struct mqtt_offload_client *client);

/** @brief Closes the transport connection.
 *
 *  @param client Pointer to the MQTT offload client.
 *  @return 0 on success, or a negative error code.
 */
int mqtt_transport_close(struct mqtt_offload_client *client);

int mqtt_offload_mqtt_connect(struct mqtt_offload_client *client, const void *params);
int mqtt_offload_write(struct mqtt_offload_client *client, void *buf, size_t sz,
		       const void *dest_addr, size_t addrlen);
ssize_t mqtt_offload_read(struct mqtt_offload_client *client, void *rx_buf, size_t rx_len,
			  void *src_addr, size_t *addrlen);
int mqtt_offload_poll(struct mqtt_offload_client *client);
int mqtt_offload_close(struct mqtt_offload_client *client);
#if defined(CONFIG_MQTT_OFFLOAD_LIB_TLS)
/** @brief Connect handler for TLS transport.
 *
 *  @param client Pointer to the MQTT offload client.
 *  @param params TLS-specific connection parameters.
 *  @return 0 on success, or a negative error code.
 */
int mqtt_offload_tls_connect(struct mqtt_offload_client *client, const void *params);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MQTT_TRANSPORT_H_ */
