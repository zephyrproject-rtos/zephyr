/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_TYPES_H_
#define ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_TYPES_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zbus Multi-domain API
 * @defgroup zbus_multidomain_apis Zbus Multi-domain APIs
 * @ingroup zbus_apis
 * @since 3.3.0
 * @version 1.0.0
 * @ingroup os_services
 * @{
 */

/**
 * @brief Type of the proxy agent.
 *
 * This enum defines the types of proxy agents that can be used in a multi-domain
 * Zbus setup. Each type corresponds to a different communication backend.
 */
enum zbus_multidomain_type {
	ZBUS_MULTIDOMAIN_TYPE_UART,
	ZBUS_MULTIDOMAIN_TYPE_IPC
};

/**
 * @brief Type of the proxy agent message.
 *
 * This enum defines the types of messages that can be sent or received by the proxy agent.
 * A message can either be a data message or an acknowledgment (ACK) message.
 */
enum zbus_proxy_agent_msg_type {
	ZBUS_PROXY_AGENT_MSG_TYPE_MSG = 0,
	ZBUS_PROXY_AGENT_MSG_TYPE_ACK = 1,
};

/**
 * @brief Message structure for the proxy agent.
 *
 * This structure represents a message that is sent or received by the proxy agent.
 * It contains the size of the message, the actual message data, and the channel name
 * associated with the message.
 */
struct zbus_proxy_agent_msg {
	/* Type of the message: data or acknowledgment */
	uint8_t type;

	/* Message id. sys_clock_cycle when sent */
	uint32_t id;

	/* The size of the message */
	uint32_t message_size;

	/* The channel associated with the message */
	uint8_t message_data[CONFIG_ZBUS_MULTIDOMAIN_MESSAGE_SIZE];

	/* The length of the channel name */
	uint32_t channel_name_len;

	/* The name of the channel */
	char channel_name[CONFIG_ZBUS_MULTIDOMAIN_CHANNEL_NAME_SIZE];

	/* CRC32 of the message for integrity check */
	uint32_t crc32;
} __packed;

/**
 * @brief Proxy agent API structure.
 */
struct zbus_proxy_agent_api {
	/**
	 * @brief Initialize the backend for the proxy agent.
	 *
	 * This function is called to initialize the backend specific to the proxy agent.
	 *
	 * @param config Pointer to the backend specific configuration.
	 * @return int 0 on success, negative error code on failure.
	 */
	int (*backend_init)(void *config);

	/**
	 * @brief Send a message through the proxy agent.
	 *
	 * This function is called to send a message through the proxy agent.
	 *
	 * @param config Pointer to the backend specific configuration.
	 * @param msg Pointer to the message to be sent.
	 * @return int 0 on success, negative error code on failure.
	 */
	int (*backend_send)(void *config, struct zbus_proxy_agent_msg *msg);

	/**
	 * @brief Set the receive callback for the proxy agent.
	 *
	 * This function is called to set the callback function that will be invoked
	 * when a message is received by the backend.
	 *
	 * @param config Pointer to the backend specific configuration.
	 * @param recv_cb Pointer to the callback function to be set.
	 * @return int 0 on success, negative error code on failure.
	 */
	int (*backend_set_recv_cb)(void *config,
				   int (*recv_cb)(const struct zbus_proxy_agent_msg *msg));

	/**
	 * @brief Set the acknowledgment callback for the proxy agent.
	 *
	 * This function is called to set the callback function that will be invoked
	 * when an acknowledgment is received for a sent message.
	 *
	 * @param config Pointer to the backend specific configuration.
	 * @param ack_cb Pointer to the acknowledgment callback function to be set.
	 * @param user_data Pointer to user data that will be passed to the acknowledgment callback.
	 * @return int 0 on success, negative error code on failure.
	 */
	int (*backend_set_ack_cb)(void *config, int (*ack_cb)(uint32_t msg_id, void *user_data),
				  void *user_data);
};

/**
 * @brief Initialize a proxy agent message with CRC
 *
 * This function initializes a zbus_proxy_agent_msg structure with the provided
 * channel and message data, automatically setting the message type, ID, and CRC.
 *
 * @param msg Pointer to the message structure to initialize
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
	if (!msg || !message_data || !channel_name || data_size == 0 ||
	    data_size > CONFIG_ZBUS_MULTIDOMAIN_MESSAGE_SIZE || channel_name_len == 0 ||
	    channel_name_len > CONFIG_ZBUS_MULTIDOMAIN_CHANNEL_NAME_SIZE) {
		return -EINVAL;
	}

	memset(msg, 0, sizeof(*msg));

	msg->type = ZBUS_PROXY_AGENT_MSG_TYPE_MSG;
	msg->id = sys_clock_cycle_get_32();
	msg->message_size = data_size;
	memcpy(msg->message_data, message_data, data_size);
	msg->channel_name_len = channel_name_len;
	strncpy(msg->channel_name, channel_name, sizeof(msg->channel_name) - 1);
	msg->channel_name[sizeof(msg->channel_name) - 1] = '\0';
	msg->crc32 = crc32_ieee((const uint8_t *)msg, sizeof(*msg) - sizeof(msg->crc32));
	return 0;
}

/**
 * @brief Initialize an ACK proxy agent message with CRC
 *
 * This function initializes a zbus_proxy_agent_msg structure as an acknowledgment (ACK)
 *
 * @param msg pointer to the message structure to initialize
 * @param msg_id id of the message being acknowledged
 * @return int 0 on success, negative error code on failure
 */
static inline int zbus_create_proxy_agent_ack_msg(struct zbus_proxy_agent_msg *msg, uint32_t msg_id)
{
	if (!msg) {
		return -EINVAL;
	}

	memset(msg, 0, sizeof(*msg));

	msg->type = ZBUS_PROXY_AGENT_MSG_TYPE_ACK;
	msg->id = msg_id;
	msg->message_size = 0;
	msg->crc32 = crc32_ieee((const uint8_t *)msg, sizeof(*msg) - sizeof(msg->crc32));
	return 0;
}

/**
 * @brief Verify the CRC32 of a proxy agent message
 *
 * This function verifies the CRC32 checksum of a given zbus_proxy_agent_msg structure.
 * It calculates the CRC32 of the message (excluding the crc32 field itself) and compares
 * it to the provided crc32 value in the structure.
 *
 * @param msg pointer to the message structure to verify
 * @return int 0 if the CRC is valid, negative error code on failure
 */
static inline int verify_proxy_agent_msg_crc(const struct zbus_proxy_agent_msg *msg)
{
	if (!msg) {
		return -EINVAL;
	}
	uint32_t calculated_crc =
		crc32_ieee((const uint8_t *)msg, sizeof(*msg) - sizeof(msg->crc32));

	if (calculated_crc != msg->crc32) {
		return -EILSEQ;
	}

	return 0;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_TYPES_H_ */
