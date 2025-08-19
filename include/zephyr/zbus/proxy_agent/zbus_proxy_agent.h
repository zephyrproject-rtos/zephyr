/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_H_
#define ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_H_

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/slist.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_types.h>

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
 * @brief Backend configuration for proxy agent
 */
struct zbus_proxy_agent_backend_config {
	/** The name of the proxy agent */
	const char *name;

	/** The type of the proxy agent */
	enum zbus_proxy_agent_type type;

	/** Pointer to the backend API */
	const struct zbus_proxy_agent_backend_api *backend_api;

	/** Pointer to the backend specific configuration */
	void *backend_config;
};

/**
 * @brief Message tracking configuration for proxy agent
 */
struct zbus_proxy_agent_tracking_config {
	/** Pool for tracking sent messages awaiting acknowledgment */
	struct net_buf_pool *tracking_msg_pool;

	/** List of sent messages awaiting acknowledgment */
	sys_slist_t tracking_msg_list;

	/** Initial acknowledgment timeout in milliseconds */
	int32_t ack_timeout_initial_ms;

	/** Maximum number of acknowledgment attempts */
	int32_t ack_attempt_limit;

	/** Message queue for tracking cleanup */
	struct k_msgq cleanup_msgq;

	/** Buffer for the cleanup message queue */
	char *cleanup_msgq_buffer;
};

/**
 * @brief Structure for tracking sent messages awaiting acknowledgment.
 */
struct zbus_proxy_agent_tracked_msg {
	/** Copy of the sent message */
	struct zbus_proxy_agent_msg msg;

	/** Pointer to the proxy agent configuration */
	struct zbus_proxy_agent_config *config;

	/** Number of transmit attempts made for this message */
	uint8_t transmit_attempts;

	/** Work item for handling acknowledgment timeout */
	struct k_work_delayable work;

	/** Atomic flag indicating ACK/NACK has been received */
	atomic_t ack_nack_received;
};

/**
 * @brief Receive path configuration for proxy agent
 */
struct zbus_proxy_agent_receive_config {
	/** Message queue for received messages */
	struct k_msgq receive_msgq;

	/** Buffer for the receive message queue */
	char *receive_msgq_buffer;

	/** Size of the receive message queue buffer */
	size_t receive_msgq_buffer_size;
};

/**
 * @brief Response handling state for proxy agent
 */
struct zbus_proxy_agent_response_state {
	/** Work item for sending ACK/NACK responses asynchronously */
	struct k_work response_work;

	/** Message ID for pending response */
	uint32_t pending_response_msg_id;

	/** Type of pending response (ACK or NACK) */
	uint8_t pending_response_type;
};

/**
 * @brief Duplicate detection state for proxy agent
 */
struct zbus_proxy_agent_duplicate_detection {
	/** Duplicate detection ring buffer for received message IDs */
	uint32_t *detection_buffer;

	/** Size of the duplicate detection buffer */
	size_t detection_buffer_size;

	/** Head position in the duplicate detection ring buffer */
	uint8_t detection_head;
};

/**
 * @brief Configuration structure for the proxy agent.
 */
struct zbus_proxy_agent_config {
	/** Backend configuration */
	struct zbus_proxy_agent_backend_config backend;

	/** Message tracking and retransmission */
	struct zbus_proxy_agent_tracking_config tracking;

	/** Receive path configuration */
	struct zbus_proxy_agent_receive_config receive;

	/** Response handling state */
	struct zbus_proxy_agent_response_state response;

	/** Duplicate detection */
	struct zbus_proxy_agent_duplicate_detection duplicate_detection;

	/** Shadow channel validator function */
	bool (*shadow_validator)(const void *msg, size_t msg_size);

	/** Serialization buffer */
	uint8_t *serialization_buffer;

	/** Size of the serialization buffer */
	size_t serialization_buffer_size;
};

/**
 * @brief Set up a proxy agent using the provided configuration.
 * @hideinitializer
 *
 * Starts the proxy agent thread and initializes the necessary resources.
 * Multiple proxy agent instances can coexist in the same application,
 * using multiple backends.
 *
 * @param _name The name to give to the proxy agent instance. need to be referenced when adding
 *              channels to the proxy agent.
 * @param _type The type of the proxy agent backend (see #zbus_proxy_agent_type).
 * @param _backend_dt_node Device tree node identifier for the backend device.
 * @param _initial_timeout Initial acknowledgment timeout in milliseconds.
 * @param _transmit_attempts Maximum number of transmission attempts for unacknowledged before
 *                           giving up.
 * @param _max_concurrent_msgs Maximum number of concurrent tracked messages.
 */
#define ZBUS_PROXY_AGENT_DEFINE(_name, _type, _backend_dt_node, _initial_timeout,                  \
				_transmit_attempts, _max_concurrent_msgs)                          \
	NET_BUF_POOL_DEFINE(_name##_tracking_msg_pool, _max_concurrent_msgs,                       \
			    sizeof(struct zbus_proxy_agent_tracked_msg), sizeof(uint32_t), NULL);  \
	_ZBUS_GENERATE_BACKEND_CONFIG(_name, _type, _backend_dt_node);                             \
	static char _name##_receive_msgq_buf[CONFIG_ZBUS_PROXY_AGENT_RECEIVE_QUEUE_SIZE *          \
					       sizeof(struct zbus_proxy_agent_msg)];               \
	static uint32_t _name##_dup_detect_buf[_max_concurrent_msgs];                              \
	static uint8_t _name##_serial_buf[CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE +                   \
					  CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE + 16];         \
	static char _name##_cleanup_msgq_buf[CONFIG_ZBUS_PROXY_AGENT_CLEANUP_QUEUE_SIZE *          \
					       sizeof(uint32_t)];                                  \
	/* Forward declare the shadow validator function */                                        \
	bool _name##_shadow_validator(const void *msg, size_t msg_size);                           \
	struct zbus_proxy_agent_config _name##_config = {                                          \
		.backend =                                                                         \
			{                                                                          \
				.name = #_name,                                                    \
				.type = _type,                                                     \
				.backend_api = _ZBUS_GET_BACKEND_API(_type),                       \
				.backend_config = _ZBUS_GET_CONFIG(_name, _type),                  \
			},                                                                         \
		.tracking =                                                                        \
			{                                                                          \
				.tracking_msg_pool = &_name##_tracking_msg_pool,                   \
				.ack_timeout_initial_ms = _initial_timeout,                        \
				.ack_attempt_limit = _transmit_attempts,                           \
				.cleanup_msgq_buffer = _name##_cleanup_msgq_buf,                   \
			},                                                                         \
		.receive =                                                                         \
			{                                                                          \
				.receive_msgq_buffer = _name##_receive_msgq_buf,                   \
				.receive_msgq_buffer_size = sizeof(_name##_receive_msgq_buf),      \
			},                                                                         \
		.response =                                                                        \
			{                                                                          \
				.pending_response_msg_id = 0,                                      \
				.pending_response_type = 0,                                        \
			},                                                                         \
		.duplicate_detection =                                                             \
			{                                                                          \
				.detection_buffer = _name##_dup_detect_buf,                        \
				.detection_buffer_size =                                           \
					sizeof(_name##_dup_detect_buf) / sizeof(uint32_t),         \
				.detection_head = 0,                                               \
			},                                                                         \
		.shadow_validator = _name##_shadow_validator,                                      \
		.serialization_buffer = _name##_serial_buf,                                        \
		.serialization_buffer_size = sizeof(_name##_serial_buf),                           \
	};                                                                                         \
	ZBUS_MSG_SUBSCRIBER_DEFINE(_name##_subscriber);                                            \
	K_THREAD_DEFINE(_name##_thread_id, CONFIG_ZBUS_PROXY_AGENT_STACK_SIZE,                     \
			zbus_proxy_agent_thread, &_name##_config, &_name##_subscriber, NULL,       \
			CONFIG_ZBUS_PROXY_AGENT_PRIORITY, 0, 0);                                   \
	_ZBUS_PROXY_AGENT_GENERATE_SHADOW_VALIDATOR(_name);

/**
 * @brief Add a channel to the proxy agent.
 *
 * This macro registers a channel with the specified proxy agent so that the agent
 * will forward messages published on that channel to the remote domain.
 *
 * The same channel can be added to multiple proxy agents to enable multi-backend
 * forwarding, where messages are simultaneously sent via different transports
 * (e.g., UART and IPC).
 *
 * @param _proxy_name The name of the proxy agent to add the channel to.
 * @param _chan The channel to be added.
 */
#define ZBUS_PROXY_ADD_CHAN(_proxy_name, _chan)                                                    \
/* Forward declare the proxy agent's subscriber (generated by ZBUS_PROXY_AGENT_DEFINE) */          \
	extern const struct zbus_observer _proxy_name##_subscriber;                                \
	ZBUS_CHAN_ADD_OBS(_chan, _proxy_name##_subscriber, 0);

/**
 * @brief Define a shadow channel linked to a proxy agent.
 *
 * @note Shadow channels are local channels that mirror remote channels from other domains.
 * They can only be published to by their associated proxy agent to ensure data integrity.
 *
 * @param _name Channel name
 * @param _type Message type structure
 * @param _proxy_name The name of the linked proxy agent
 * @param _user_data User data pointer
 * @param _observers Static list of observers
 * @param _init_val Initial channel value
 */
#define ZBUS_SHADOW_CHAN_DEFINE(_name, _type, _proxy_name, _user_data, _observers, _init_val)      \
/* Forward declare the proxy's shadow validator function (generated by ZBUS_PROXY_AGENT_DEFINE) */ \
	extern bool _proxy_name##_shadow_validator(const void *msg, size_t msg_size);              \
	ZBUS_CHAN_DEFINE(_name, _type, _ZBUS_PROXY_AGENT_VALIDATOR(_proxy_name), _user_data,       \
			 _observers, _init_val)

/**
 * @brief Define a shadow channel with a specific ID linked to a proxy agent.
 *
 * @note Shadow channels are local channels that mirror remote channels from other domains.
 * They can only be published to by their associated proxy agent to ensure data integrity.
 *
 * @param _name Channel name
 * @param _id channel ID
 * @param _type Message type structure
 * @param _proxy_name The name of the linked proxy agent
 * @param _user_data User data pointer
 * @param _observers Static list of observers
 * @param _init_val Initial channel value
 */
#define ZBUS_SHADOW_CHAN_DEFINE_WITH_ID(_name, _id, _type, _proxy_name, _user_data, _observers,    \
					   _init_val)                                              \
/* Forward declare the proxy's shadow validator function (generated by ZBUS_PROXY_AGENT_DEFINE) */ \
	extern bool _proxy_name##_shadow_validator(const void *msg, size_t msg_size);              \
	ZBUS_CHAN_DEFINE_WITH_ID(_name, _id, _type, _ZBUS_PROXY_AGENT_VALIDATOR(_proxy_name),      \
				 _user_data, _observers, _init_val)

/**
 * @brief Thread function for the proxy agent.
 *
 * This function runs in a separate thread and handles the main operations of the proxy agent.
 *
 * @param config Pointer to the configuration structure for the proxy agent.
 * @param subscriber Pointer to the zbus observer that the proxy agent listens to.
 * @return negative error code on failure.
 */
int zbus_proxy_agent_thread(struct zbus_proxy_agent_config *config,
			    const struct zbus_observer *subscriber);

/** @cond INTERNAL_HIDDEN */

/*
 * Generate a shadow validator function for the specified proxy agent.
 * This validator ensures that shadow channels can only be published to by their
 * associated proxy agent thread, maintaining data integrity across domains.
 */
#define _ZBUS_PROXY_AGENT_GENERATE_SHADOW_VALIDATOR(_proxy_name)                                   \
	bool _proxy_name##_shadow_validator(const void *msg, size_t msg_size)                      \
	{                                                                                          \
		k_tid_t proxy_thread_id;                                                           \
												   \
		proxy_thread_id = _proxy_name##_thread_id;                                         \
		CHECKIF(proxy_thread_id == NULL) {                                                 \
			LOG_ERR("Proxy agent thread ID is NULL");                                  \
			return false;                                                              \
		}                                                                                  \
		if (k_current_get() != proxy_thread_id) {                                          \
			LOG_ERR("Shadow channel can only be published by the proxy agent");        \
			return false;                                                              \
		}                                                                                  \
		return true;                                                                       \
	}

/**
 * @brief Macro to reference the shadow validator function for a proxy agent.
 */
#define _ZBUS_PROXY_AGENT_VALIDATOR(_proxy_name) _proxy_name##_shadow_validator

/**
 * @brief Backend configuration generation macros.
 *
 * These macros use token pasting to get the backend-specific configuration macro
 * Allows for automatic generation of backend configurations based on type.
 */
#define _ZBUS_GENERATE_BACKEND_CONFIG(_name, _type, _node_id)                                      \
	_ZBUS_GENERATE_BACKEND_CONFIG_##_type(_name, _node_id)

/**
 * @brief Macros to get the backend API and configuration based on type.
 *
 * These macros use token pasting to retrieve the appropriate backend API
 * and configuration macros for the specified proxy agent type.
 * Allows for automatic selection of backend implementations.
 */
#define _ZBUS_GET_BACKEND_API(_type)   _ZBUS_GET_BACKEND_API_##_type()
#define _ZBUS_GET_CONFIG(_name, _type) _ZBUS_GET_CONFIG_##_type(_name)

/**
 * @brief Buffer size needed to serialize ACK/NACK response messages (header only).
 *
 * ACK/NACK messages do not contain message data or channel name, so the buffer size
 * is calculated based on the sizes of the type, id, message_size, and channel_name_len fields only.
 */
#define ZBUS_PROXY_AGENT_RESPONSE_BUFFER_SIZE                                                      \
	(sizeof(((struct zbus_proxy_agent_msg *)0)->type) +                                        \
	 sizeof(((struct zbus_proxy_agent_msg *)0)->id) +                                          \
	 sizeof(((struct zbus_proxy_agent_msg *)0)->message_size) +                                \
	 sizeof(((struct zbus_proxy_agent_msg *)0)->channel_name_len))

/** @endcond */

/**
 * @brief Helper function to serialize a proxy agent message into a byte buffer.
 *
 * This function converts a zbus_proxy_agent_msg structure into a contiguous byte array removing
 * unused space for easier transmission over transport layers.
 *
 * @param msg Pointer to the message to serialize
 * @param buffer Pointer to the output buffer
 * @param buffer_size Size of the output buffer
 * @return size_t Number of bytes written to the buffer, or negative error code on failure
 */
static inline int serialize_proxy_agent_msg(const struct zbus_proxy_agent_msg *msg,
					    uint8_t *buffer, size_t buffer_size)
{
	int total_size;
	int offset = 0;

	_ZBUS_ASSERT(msg != NULL, "msg cannot be NULL");
	_ZBUS_ASSERT(buffer != NULL, "buffer cannot be NULL");
	_ZBUS_ASSERT(buffer_size > 0, "buffer_size must be positive");

	total_size = sizeof(msg->type) + sizeof(msg->id) + sizeof(msg->message_size) +
		     msg->message_size + sizeof(msg->channel_name_len) + msg->channel_name_len;

	_ZBUS_ASSERT(buffer_size >= total_size,
		     "buffer_size is too small for serialized message");

	memcpy(buffer + offset, &msg->type, sizeof(msg->type));
	offset += sizeof(msg->type);
	memcpy(buffer + offset, &msg->id, sizeof(msg->id));
	offset += sizeof(msg->id);
	memcpy(buffer + offset, &msg->message_size, sizeof(msg->message_size));
	offset += sizeof(msg->message_size);
	memcpy(buffer + offset, msg->message_data, msg->message_size);
	offset += msg->message_size;
	memcpy(buffer + offset, &msg->channel_name_len, sizeof(msg->channel_name_len));
	offset += sizeof(msg->channel_name_len);
	memcpy(buffer + offset, msg->channel_name, msg->channel_name_len);
	offset += msg->channel_name_len;
	return offset;
}

/**
 * @brief Helper function to deserialize a byte buffer into a proxy agent message.
 *
 * This function converts a contiguous byte array into a zbus_proxy_agent_msg structure,
 * reconstructing the message from the serialized format.
 *
 * @param buffer Pointer to the input byte buffer
 * @param buffer_size Size of the input buffer
 * @param msg Pointer to the output message structure
 * @return int 0 on success, or negative error code on failure
 */
static inline int deserialize_proxy_agent_msg(const uint8_t *buffer, size_t buffer_size,
					      struct zbus_proxy_agent_msg *msg)
{
	_ZBUS_ASSERT(buffer != NULL, "buffer cannot be NULL");
	_ZBUS_ASSERT(buffer_size > 0, "buffer_size must be positive");
	_ZBUS_ASSERT(msg != NULL, "msg cannot be NULL");

	size_t offset = 0;

	memcpy(&msg->type, buffer + offset, sizeof(msg->type));
	offset += sizeof(msg->type);
	memcpy(&msg->id, buffer + offset, sizeof(msg->id));
	offset += sizeof(msg->id);
	memcpy(&msg->message_size, buffer + offset, sizeof(msg->message_size));
	offset += sizeof(msg->message_size);
	/* Validate message size */
	if (msg->message_size > CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE) {
		return -EINVAL;
	}
	if (offset + msg->message_size + sizeof(msg->channel_name_len) > buffer_size) {
		return -ENOMEM;
	}
	memcpy(msg->message_data, buffer + offset, msg->message_size);
	offset += msg->message_size;
	memcpy(&msg->channel_name_len, buffer + offset, sizeof(msg->channel_name_len));
	offset += sizeof(msg->channel_name_len);
	/* Validate channel name length */
	if (msg->channel_name_len > CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE) {
		return -EINVAL;
	}
	if (offset + msg->channel_name_len > buffer_size) {
		return -ENOMEM;
	}
	memcpy(msg->channel_name, buffer + offset, msg->channel_name_len);
	offset += msg->channel_name_len;
	/* Null-terminate channel name */
	if (msg->channel_name_len < CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE) {
		msg->channel_name[msg->channel_name_len] = '\0';
	} else {
		msg->channel_name[CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE - 1] = '\0';
	}

	return 0;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_H_ */
