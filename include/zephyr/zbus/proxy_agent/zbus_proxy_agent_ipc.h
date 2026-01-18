/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_IPC_H_
#define ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_IPC_H_

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zbus Multi-domain API
 * @defgroup zbus_proxy_agent_ipc_backend Zbus Proxy Agent IPC Backend
 * @ingroup zbus_proxy_agent
 * @{
 */

/**
 * @brief Structure for IPC backend configuration.
 */
struct zbus_proxy_agent_ipc_config {
	/** IPC device */
	const struct device *dev;

	/** IPC endpoint */
	struct ipc_ept ipc_ept;

	/** IPC endpoint configuration */
	struct ipc_ept_cfg *ept_cfg;

	/** Semaphore to signal when the IPC endpoint is bound */
	struct k_sem ept_bound_sem;

	/** Callback function for received data */
	int (*recv_cb)(const uint8_t *data, size_t length, void *user_data);

	/** User data for the receive callback */
	void *recv_cb_user_data;
};

/**
 * @brief Transport message structure with CRC for backend communication.
 */
struct zbus_proxy_agent_ipc_msg {
	uint8_t payload[CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE +
			CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE + 64];
	uint32_t crc32;
} __packed;

/** @cond INTERNAL_HIDDEN */

/* IPC backend API structure */
extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;

/**
 * @brief Backend dispatch macros for the IPC backend.
 *
 * These macros follow the standard backend implementation pattern defined in
 * zbus_proxy_agent.h. They provide API and configuration access for the IPC
 * communication backend.
 */
#define _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_IPC() &zbus_proxy_agent_ipc_backend_api
#define _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_IPC(_name) (void *)&_name##_ipc_config

/**
 * @brief Macro to generate device specific backend configurations for the IPC type.
 *
 * This macro generates the backend specific configurations for the IPC type of
 * proxy agent. The macro is used in "zbus_proxy_agent.h" to create the
 * backend specific configurations for the IPC type of proxy agent.
 *
 * @param _name The name of the proxy agent (used as identifier prefix).
 * @param _node The device tree node for the IPC device.
 */
#define _ZBUS_GENERATE_BACKEND_CONFIG_ZBUS_PROXY_AGENT_TYPE_IPC(_name, _node)                   \
	static struct ipc_ept_cfg _name##_ipc_ept_cfg = {                                          \
		.name = "ipc_ept_" #_name,                                                         \
	};                                                                                         \
	static struct zbus_proxy_agent_ipc_config _name##_ipc_config = {                           \
		.dev = DEVICE_DT_GET(_node),                                                    \
		.ept_cfg = &_name##_ipc_ept_cfg,                                                   \
	}

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_IPC_H_ */
