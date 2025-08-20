/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_H_
#define ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>
#include <string.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/slist.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/multidomain/zbus_multidomain_types.h>

#if defined(CONFIG_ZBUS_MULTIDOMAIN_UART)
#include <zephyr/zbus/multidomain/zbus_multidomain_uart.h>
#endif
#if defined(CONFIG_ZBUS_MULTIDOMAIN_IPC)
#include <zephyr/zbus/multidomain/zbus_multidomain_ipc.h>
#endif

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
 * @brief Structure for tracking sent messages awaiting acknowledgment.
 *
 * This structure is used internally by the proxy agent to keep track of messages
 * that have been sent but not yet acknowledged. It contains a copy of the message,
 * the number of transmit attempts, and a delayed work item for timeout handling.
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
};

/**
 * @brief Configuration structure for the proxy agent.
 *
 * This structure holds the configuration for a proxy agent, including its name,
 * type, backend specific API, and backend specific configuration.
 */
struct zbus_proxy_agent_config {
	/* The name of the proxy agent */
	const char *name;

	/* The type of the proxy agent */
	enum zbus_multidomain_type type;

	/* Pointer to the backend specific API */
	const struct zbus_proxy_agent_api *api;

	/* Pointer to the backend specific configuration */
	void *backend_config;

	/* Pool for tracking sent messages awaiting acknowledgment */
	struct net_buf_pool *sent_msg_pool;

	/* List of sent messages awaiting acknowledgment */
	sys_slist_t sent_msg_list;
};

/**
 * @brief Set up a proxy agent using the provided configuration.
 *
 * Starts the proxy agent thread and initializes the necessary resources.
 *
 * @note This macro sets up net_buf_pool for tracking sent messages, defines
 * a zbus subscriber, and creates a thread for the proxy agent.
 *
 * @note the ZBUS_MULTIDOMAIN_SENT_MSG_POOL_SIZE configuration option
 * must be set to a value greater than or equal to the maximum number of
 * unacknowledged messages that can be in flight at any given time.
 *
 * @note The configuration options ZBUS_MULTIDOMAIN_PROXY_STACK_SIZE and
 * ZBUS_MULTIDOMAIN_PROXY_PRIORITY define the stack size and priority of the
 * proxy agent thread, respectively.
 *
 * @param _name The name of the proxy agent.
 * @param _type The type of the proxy agent (enum zbus_multidomain_type)
 * @param _nodeid The device node ID for the proxy agent.
 */
#define ZBUS_PROXY_AGENT_DEFINE(_name, _type, _nodeid)                                             \
	NET_BUF_POOL_DEFINE(_name##_sent_msg_pool, CONFIG_ZBUS_MULTIDOMAIN_SENT_MSG_POOL_SIZE,     \
			    sizeof(struct zbus_proxy_agent_tracked_msg), sizeof(uint32_t), NULL);  \
	_ZBUS_GENERATE_BACKEND_CONFIG(_name, _type, _nodeid);                                      \
	struct zbus_proxy_agent_config _name##_config = {                                          \
		.name = #_name,                                                                    \
		.type = _type,                                                                     \
		.api = _ZBUS_GET_API(_type),                                                       \
		.backend_config = _ZBUS_GET_CONFIG(_name, _type),                                  \
		.sent_msg_pool = &_name##_sent_msg_pool,                                           \
	};                                                                                         \
	ZBUS_MSG_SUBSCRIBER_DEFINE(_name##_subscriber);                                            \
	K_THREAD_DEFINE(_name##_thread_id, CONFIG_ZBUS_MULTIDOMAIN_PROXY_STACK_SIZE,               \
			zbus_proxy_agent_thread, &_name##_config, &_name##_subscriber, NULL,       \
			CONFIG_ZBUS_MULTIDOMAIN_PROXY_PRIORITY, 0, 0);

/**
 * @brief Add a channel to the proxy agent.
 *
 * @param _name The name of the proxy agent.
 * @param _chan The channel to be added.
 */
#define ZBUS_PROXY_ADD_CHANNEL(_name, _chan) ZBUS_CHAN_ADD_OBS(_chan, _name##_subscriber, 0);

/**
 * @brief Thread function for the proxy agent.
 *
 * This function runs in a separate thread and continuously listens for messages
 * on the zbus observer. It processes incoming messages and forwards them
 * to the appropriate backend for sending.
 *
 * @param config Pointer to the configuration structure for the proxy agent.
 * @param subscriber Pointer to the zbus observer that the proxy agent listens to.
 * @return negative error code on failure.
 */
int zbus_proxy_agent_thread(struct zbus_proxy_agent_config *config,
			    const struct zbus_observer *subscriber);

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Macros to generate backend specific configurations for the proxy agent.
 *
 * This macro generates the backend specific configurations based on the type of
 * the proxy agent.
 *
 * @param _name The name of the proxy agent.
 * @param _type The type of the proxy agent (enum zbus_multidomain_type).
 * @param _nodeid The device node ID for the proxy agent.
 *
 * @note This macro finds the matching backend configuration macro from the
 * backend specific header files. Requires the backend specific header files to
 * define the macros in the format `_ZBUS_GENERATE_BACKEND_CONFIG_<type>(_name, _nodeid)`.
 */
#define _ZBUS_GENERATE_BACKEND_CONFIG(_name, _type, _nodeid)                                       \
	_ZBUS_GENERATE_BACKEND_CONFIG_##_type(_name, _nodeid)

/**
 * @brief Generic macros to get the API and configuration for the specified type of proxy agent.
 *
 * These macros are used to retrieve the API and configuration for the specified type of
 * proxy agent. The type is specified as an argument to the macro.
 *
 * @param _type The type of the proxy agent (enum zbus_multidomain_type).
 * @param _name The name of the proxy agent.
 *
 * @note These macros are used to retrieve the API and configuration for the specified type of
 * proxy agent. Requires the backend specific header files to define the macros in the format
 * `_ZBUS_GET_API_<type>()` and `_ZBUS_GET_CONFIG_<name, type>()`.
 */
#define _ZBUS_GET_API(_type)           _ZBUS_GET_API_##_type()
#define _ZBUS_GET_CONFIG(_name, _type) _ZBUS_GET_CONFIG_##_type(_name)

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_H_ */
