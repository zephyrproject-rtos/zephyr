/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_H_
#define ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file zbus_proxy_agent.h
 * @brief Zbus Multi-domain API
 * @defgroup zbus_proxy_agent Zbus Proxy Agent API
 * @ingroup zbus_apis
 * @{
 */

/* Forward declarations */
struct zbus_proxy_agent_backend_api;
struct zbus_proxy_agent;

/**
 * @brief Enum for supported proxy agent backend types.
 */
enum zbus_proxy_agent_backend_type {
	/** IPC backend for inter-process communication */
	ZBUS_PROXY_AGENT_BACKEND_IPC,
};

/**
 * @brief Metadata for shadow channels to track their ownership by proxy agents.
 */
struct zbus_shadow_channel {
	/** Pointer to the shadow channel */
	const struct zbus_channel *chan;
	/** Pointer to the proxy agent that owns this shadow channel */
	const struct zbus_proxy_agent *proxy_agent;
};

/**
 * @brief Structure for proxy agent message.
 *
 * @note This type defines the proxy-agent transport frame with fixed-size buffers.
 * Backends should consider serializing/compacting messages over the wire to minimize transfer size
 */
struct zbus_proxy_msg {
	/** Size of the message data */
	uint32_t message_size;
	/** Binary message data */
	uint8_t message[CONFIG_ZBUS_PROXY_AGENT_MAX_MESSAGE_SIZE];
	/** Length of the channel name including the NUL terminator */
	uint32_t channel_name_len;
	/** Channel name associated with the message */
	char channel_name[CONFIG_ZBUS_PROXY_AGENT_MAX_CHANNEL_NAME_SIZE];
};

/**
 * @brief Internal thread payload for validated received messages.
 */
struct zbus_proxy_agent_rx_msg {
	/** Resolved shadow channel that will receive the message */
	const struct zbus_channel *chan;
	/** Binary message data */
	uint8_t message[CONFIG_ZBUS_PROXY_AGENT_MAX_MESSAGE_SIZE];
};

/**
 * @brief Structure for proxy agent configuration.
 *
 * This structure represents a proxy agent instance and its associated configuration.
 */
struct zbus_proxy_agent {
	/** The name of the proxy agent */
	const char *name;
	/** The type of the proxy agent backend */
	enum zbus_proxy_agent_backend_type type;
	/** Backend specific configuration */
	const void *backend_config;
	/** Backend API determined at compile-time based on backend type */
	const struct zbus_proxy_agent_backend_api *backend_api;
	/** Dedicated thread used to process received proxy messages */
	struct k_thread *thread;
	/** msgq for received proxy messages */
	struct k_msgq *msgq;
	/** Stack for the dedicated proxy agent thread */
	k_thread_stack_t *stack;
	/** Pointer to the thread ID of the dedicated proxy agent thread */
	k_tid_t *const thread_id;
};

/**
 * @brief Type definition for the proxy agent receive callback function.
 *
 * This callback is invoked when a proxy agent receives a message. The callback
 * should process the message and return 0 on success or a negative error code
 * on failure.
 *
 * @param agent Pointer to the proxy agent instance
 * @param msg Pointer to the received proxy message
 * @return int 0 on success, negative error code on failure
 */
typedef int (*zbus_proxy_agent_recv_cb_t)(const struct zbus_proxy_agent *agent,
					  const struct zbus_proxy_msg *msg);

/**
 * @brief Backend API structure for proxy agent backends.
 */
struct zbus_proxy_agent_backend_api {
	/** Initializer function for the backend */
	int (*backend_init)(const struct zbus_proxy_agent *agent);
	/** Send function for the backend */
	int (*backend_send)(const struct zbus_proxy_agent *agent, struct zbus_proxy_msg *msg);
	/** Function to set the receive callback for incoming messages */
	int (*backend_set_recv_cb)(const struct zbus_proxy_agent *agent,
				   zbus_proxy_agent_recv_cb_t recv_cb);
};

/**
 * @brief Structure for proxy agent backend descriptor.
 */
struct zbus_proxy_agent_backend_desc {
	/** Type of the backend */
	enum zbus_proxy_agent_backend_type type;
	/** API for the backend */
	const struct zbus_proxy_agent_backend_api *api;
};

/**
 * @brief Macro to define a proxy agent backend and register it in the iterable section.
 *
 * @param _name Name of the backend instance
 * @param _type Backend type (e.g., ZBUS_PROXY_AGENT_BACKEND_IPC)
 * @param _api Pointer to the backend API structure
 */
#define ZBUS_PROXY_AGENT_BACKEND_DEFINE(_name, _type, _api)                                        \
	STRUCT_SECTION_ITERABLE(zbus_proxy_agent_backend_desc, _name) = {                          \
		.type = (_type),                                                                   \
		.api = (_api),                                                                     \
	}

/**
 * @brief Internal proxy agent listener callback.
 *
 * @note This function is called by the generated listener callback and should not be called
 * directly.
 *
 * @param agent Proxy agent configuration
 * @param chan Channel that triggered the callback
 */
void zbus_proxy_agent_listener_cb(const struct zbus_channel *chan,
				  const struct zbus_proxy_agent *agent);

/**
 * @brief Internal proxy agent receive callback.
 *
 * @note This function is called by the generated initialization function from sys_init and should
 * not be called directly.
 *
 * Initializes the proxy agent by setting up the backend and starting the dedicated thread.
 *
 * @param agent Proxy agent configuration
 * @return int 0 on success, negative error code on failure
 */
int zbus_init_proxy_agent(const struct zbus_proxy_agent *agent);

/**
 * @brief Proxy agent definition macro.
 * @hideinitializer
 *
 * This macro defines a proxy agent with the specified name, backend type, and backend device tree
 * node.
 *
 * @param _name Name of the proxy agent instance
 * @param _type Backend type for the proxy agent (e.g., ZBUS_PROXY_AGENT_BACKEND_IPC)
 * @param _backend_dt_node Device tree node for the backend configuration
 */
#define ZBUS_PROXY_AGENT_DEFINE(_name, _type, _backend_dt_node)                                    \
	_ZBUS_PROXY_AGENT_BACKEND_CONFIG_DEFINE(_name, _type, _backend_dt_node)                    \
	K_THREAD_STACK_DEFINE(_name##_thread_stack,                                                \
			      CONFIG_ZBUS_PROXY_AGENT_WORK_QUEUE_STACK_SIZE);                      \
	K_MSGQ_DEFINE(_name##_rx_msgq, sizeof(struct zbus_proxy_agent_rx_msg),                     \
		      CONFIG_ZBUS_PROXY_AGENT_RX_QUEUE_DEPTH, 4);                                  \
	static struct k_thread _name##_thread;                                                     \
	static k_tid_t _name##_thread_id;                                                          \
	const struct zbus_proxy_agent _name = {                                                    \
		.name = #_name,                                                                    \
		.type = _type,                                                                     \
		.backend_config = _ZBUS_PROXY_AGENT_GET_BACKEND_CONFIG(_name, _type),              \
		.backend_api = _ZBUS_PROXY_AGENT_GET_BACKEND_API(_type),                           \
		.thread = &_name##_thread,                                                         \
		.msgq = &_name##_rx_msgq,                                                          \
		.stack = _name##_thread_stack,                                                     \
		.thread_id = &_name##_thread_id,                                                   \
	};                                                                                         \
	static void _name##_zbus_listener_cb(const struct zbus_channel *chan)                      \
	{                                                                                          \
		zbus_proxy_agent_listener_cb(chan, &_name);                                        \
	}                                                                                          \
	ZBUS_LISTENER_DEFINE(_name##_listener, _name##_zbus_listener_cb);                          \
	_ZBUS_PROXY_AGENT_GENERATE_SHADOW_VALIDATOR(_name);                                        \
	static int _name##_init(void)                                                              \
	{                                                                                          \
		return zbus_init_proxy_agent(&_name);                                              \
	}                                                                                          \
	SYS_INIT(_name##_init, APPLICATION, CONFIG_ZBUS_PROXY_AGENT_INIT_PRIORITY)

/** @cond INTERNAL_HIDDEN */

/** @brief Backend config generation (dispatches to per-backend macro)
 *
 * @param _name Name of the backend config instance
 * @param _type Backend type from the enum zbus_proxy_agent_backend_type
 * @param _backend_dt_node Device tree node for the backend configuration
 */
#define _ZBUS_PROXY_AGENT_BACKEND_CONFIG_DEFINE(_name, _type, _backend_dt_node)                    \
	_ZBUS_PROXY_AGENT_BACKEND_CONFIG_DEFINE_##_type(_name, _backend_dt_node)

/** @brief Backend config retrieval (dispatches to per-backend macro)
 *
 * @param _name Name of the backend config instance
 * @param _type Backend type from the enum zbus_proxy_agent_backend_type
 * @return Pointer to the backend config structure for the specified instance and type
 */
#define _ZBUS_PROXY_AGENT_GET_BACKEND_CONFIG(_name, _type)                                         \
	_ZBUS_PROXY_AGENT_GET_BACKEND_CONFIG_##_type(_name)

/** @brief Backend API resolution (dispatches to per-backend macro)
 *
 * @param _type Backend type from the enum zbus_proxy_agent_backend_type
 * @return Pointer to the backend API structure for the specified type
 */
#define _ZBUS_PROXY_AGENT_GET_BACKEND_API(_type) _ZBUS_PROXY_AGENT_GET_BACKEND_API_##_type

/** @brief Internal helper invoked when shadow channel publish validation fails.
 *
 * @note This function is used by generated validator code to report failure.
 *
 * @param agent Proxy agent configuration
 */
void zbus_proxy_agent_log_shadow_pub_denied(const struct zbus_proxy_agent *agent);

/**
 * @brief Macro to reference the shadow validator function for a proxy agent.
 *
 * @param _proxy_name Name of the proxy agent instance
 * @return Function pointer to the shadow validator function for the specified proxy agent
 */
#define _ZBUS_PROXY_AGENT_VALIDATOR(_proxy_name) _proxy_name##_shadow_validator

/**
 * @brief Generate a shadow validator function for the specified proxy agent.
 *
 * @note This validator ensures that shadow channels can only be published to by their
 * associated proxy agent's dedicated thread context.
 *
 * @param _proxy_name Name of the proxy agent instance
 */
#define _ZBUS_PROXY_AGENT_GENERATE_SHADOW_VALIDATOR(_proxy_name)                                   \
	bool _proxy_name##_shadow_validator(const void *msg, size_t msg_size)                      \
	{                                                                                          \
		(void)msg;                                                                         \
		(void)msg_size;                                                                    \
		/* Only allow publishing from the proxy agent's dedicated thread */                \
		if (k_current_get() != *_proxy_name.thread_id) {                                   \
			zbus_proxy_agent_log_shadow_pub_denied(&_proxy_name);                      \
			return false;                                                              \
		}                                                                                  \
		return true;                                                                       \
	}

/** @endcond */

/**
 * @brief Add a channel to be forwarded by the proxy agent.
 *
 * @param _agent The proxy agent instance that will forward the channel
 * @param _chan The channel to be forwarded by the proxy agent
 *
 * @note This macro adds the observer with the priority "zz". as the observer priority is
 * sorted lexicographically in the iterable section, this ensures that the proxy agent listener
 * is called after all other observers for the channel, avoiding potential blocking behavior.
 * When adding other observers for the same channel, their priority should be set to a value
 * lexicographically smaller than "zz" to ensure they are called before the proxy agent listener.
 */
#define ZBUS_PROXY_ADD_CHAN(_agent, _chan)                                                         \
	ZBUS_CHAN_ADD_OBS(_chan, _agent##_listener, zz)

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Register a shadow channel in an iterable section to link it to its proxy agent.
 *
 * @param _name Name of the shadow channel
 * @param _proxy_name Name of the proxy agent instance the shadow channel is linked to
 */
#define _ZBUS_SHADOW_CHAN_REGISTER(_name, _proxy_name)                                             \
	extern const struct zbus_proxy_agent _proxy_name;                                          \
	static const STRUCT_SECTION_ITERABLE(zbus_shadow_channel,                                  \
					     _CONCAT(_zbus_shadow_chan_, _name)) = {               \
		.chan = &_name,                                                                    \
		.proxy_agent = &_proxy_name,                                                       \
	}

/** @endcond */

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
	ZBUS_CHAN_DEFINE(_name, _type, _ZBUS_PROXY_AGENT_VALIDATOR(_proxy_name), _user_data,       \
			 _observers, _init_val);                                                   \
	_ZBUS_SHADOW_CHAN_REGISTER(_name, _proxy_name)

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
					_init_val)                                                 \
	ZBUS_CHAN_DEFINE_WITH_ID(_name, _id, _type, _ZBUS_PROXY_AGENT_VALIDATOR(_proxy_name),      \
				 _user_data, _observers, _init_val);                               \
	_ZBUS_SHADOW_CHAN_REGISTER(_name, _proxy_name)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_H_ */
