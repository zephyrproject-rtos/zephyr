/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_TYPES_H_
#define ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_TYPES_H_

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zbus Multi-domain API
 * @defgroup zbus_proxy_agent Zbus Proxy Agent API
 * @ingroup zbus_apis
 * @{
 */

/**
 * @brief Type of the proxy agent.
 */
enum zbus_proxy_agent_type {
	ZBUS_PROXY_AGENT_TYPE_IPC,
	ZBUS_PROXY_AGENT_TYPE_UART
};

/**
 * @brief Type of the proxy agent message.
 *
 * This enum defines the types of messages that can be sent or received by the proxy agent.
 * A message can be a data message, positive acknowledgment (ACK), or negative acknowledgment
 * (NACK).
 */
enum zbus_proxy_agent_msg_type {
	ZBUS_PROXY_AGENT_MSG_TYPE_MSG,
	ZBUS_PROXY_AGENT_MSG_TYPE_ACK,
	ZBUS_PROXY_AGENT_MSG_TYPE_NACK
};

/**
 * @brief Message structure for the proxy agent.
 *
 * This structure represents a message that is sent or received by the proxy agent.
 */
struct zbus_proxy_agent_msg {
	/* Type of the message: data or acknowledgment */
	uint8_t type;

	/* Message id. sys_clock_cycle when sent */
	uint32_t id;

	/* The size of the message */
	uint32_t message_size;

	/* The actual message data */
	uint8_t message_data[CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE];

	/* The length of the channel name */
	uint32_t channel_name_len;

	/* The name of the channel */
	char channel_name[CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE];

} __packed;

/**
 * @brief Backend API structure for the proxy agent.
 */
struct zbus_proxy_agent_backend_api {
	/**
	 * @brief Initialize the backend for the proxy agent.
	 *
	 * @param backend_config Pointer to the backend specific configuration.
	 * @return 0 on success, negative error code on failure.
	 */
	int (*backend_init)(void *backend_config);

	/**
	 * @brief Send data through the backend.
	 *
	 * The backend is responsible for adding integrity checks and the actual transmission.
	 *
	 * @param backend_config Pointer to the backend specific configuration.
	 * @param data Pointer to the data to be sent.
	 * @param length Length of the data to be sent.
	 * @return 0 on success, negative error code on failure.
	 */
	int (*backend_send)(void *backend_config, uint8_t *data, size_t length);

	/**
	 * @brief Set the receive callback for data.
	 *
	 * The backend will call this callback with verified data ( Integrity checked by backend).
	 *
	 * @param backend_config Pointer to the backend specific configuration.
	 * @param recv_cb Callback function for received data.
	 * @param user_data User data to pass to the callback.
	 * @return 0 on success, negative error code on failure.
	 */
	int (*backend_set_recv_cb)(void *backend_config,
				   int (*recv_cb)(const uint8_t *data, size_t length,
						  void *user_data),
				   void *user_data);
};

/**
 * @brief Create a proxy agent message with the given parameters.
 *
 * @param msg Pointer to the message structure to fill
 * @param message_data Pointer to the message data to include in the message
 * @param data_size Size of the message data in bytes
 * @param channel_name Pointer to the name of the channel associated with the message
 * @param channel_name_len Length of the channel name in bytes
 * @return 0 on success, negative error code on failure
 */
static inline int zbus_create_proxy_agent_msg(struct zbus_proxy_agent_msg *msg, void *message_data,
					      size_t data_size, const char *channel_name,
					      size_t channel_name_len)
{
	_ZBUS_ASSERT(msg != NULL, "msg cannot be NULL");
	_ZBUS_ASSERT(message_data != NULL, "message_data cannot be NULL");
	_ZBUS_ASSERT(channel_name != NULL, "channel_name cannot be NULL");
	_ZBUS_ASSERT(data_size > 0 && data_size <= CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE,
		     "data_size is out of valid range");
	_ZBUS_ASSERT(channel_name_len > 0 &&
		     channel_name_len <= CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE,
		     "channel_name_len is out of valid range");

	memset(msg, 0, sizeof(*msg));

	msg->type = ZBUS_PROXY_AGENT_MSG_TYPE_MSG;
	msg->id = sys_clock_cycle_get_32();
	msg->message_size = data_size;
	memcpy(msg->message_data, message_data, data_size);
	msg->channel_name_len = channel_name_len;
	strncpy(msg->channel_name, channel_name, sizeof(msg->channel_name) - 1);
	msg->channel_name[sizeof(msg->channel_name) - 1] = '\0';
	return 0;
}

/**
 * @brief Create an ACK message for a given message ID
 *
 * @param msg pointer to the message structure to initialize
 * @param msg_id id of the message being acknowledged
 * @return int 0 on success, negative error code on failure
 */
static inline int zbus_create_proxy_agent_ack_msg(struct zbus_proxy_agent_msg *msg, uint32_t msg_id)
{
	_ZBUS_ASSERT(msg != NULL, "msg cannot be NULL");

	memset(msg, 0, sizeof(*msg));

	msg->type = ZBUS_PROXY_AGENT_MSG_TYPE_ACK;
	msg->id = msg_id;
	msg->message_size = 0;
	return 0;
}

/**
 * @brief Create a NACK message for a given message ID
 *
 * @param msg Pointer to the message structure to be filled
 * @param msg_id The ID of the message being negatively acknowledged
 * @return 0 on success, negative error code on failure
 */
static inline int zbus_create_proxy_agent_nack_msg(struct zbus_proxy_agent_msg *msg,
						   uint32_t msg_id)
{
	_ZBUS_ASSERT(msg != NULL, "msg cannot be NULL");

	memset(msg, 0, sizeof(*msg));

	msg->type = ZBUS_PROXY_AGENT_MSG_TYPE_NACK;
	msg->id = msg_id;
	msg->message_size = 0;
	return 0;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_TYPES_H_ */
