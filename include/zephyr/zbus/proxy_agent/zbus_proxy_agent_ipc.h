/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_IPC_H_
#define ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_IPC_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file zbus_proxy_agent_ipc.h
 * @brief Zbus Multi-domain API
 * @defgroup zbus_proxy_agent_ipc_backend Zbus Proxy Agent IPC Backend
 * @ingroup zbus_proxy_agent
 *
 * @details The IPC backend uses a shared-memory message passing model via
 * `ipc_service`, and therefore transfers fixed-size proxy frames
 * (`struct zbus_proxy_msg`) between domains. This means on-wire transfer size is
 * equal to `sizeof(struct zbus_proxy_msg)` even when `message_size` or
 * `channel_name_len` are smaller. Because this frame size is determined at compile time from
 * `CONFIG_ZBUS_PROXY_AGENT_MAX_MESSAGE_SIZE` and
 * `CONFIG_ZBUS_PROXY_AGENT_MAX_CHANNEL_NAME_SIZE`, all communicating domains must use identical
 * values for these options. The IPC backend validates incoming data against
 * `sizeof(struct zbus_proxy_msg)`, so mismatched configuration across domains will cause
 * received messages to be rejected.
 * @{
 */

/**
 * @brief Mutable runtime data for the IPC backend of a Zbus proxy agent.
 */
struct zbus_proxy_agent_ipc_data {
	/** IPC endpoint */
	struct ipc_ept ipc_ept;
	/** IPC endpoint configuration */
	struct ipc_ept_cfg ept_cfg;
	/** Semaphore to signal when the IPC endpoint is bound */
	struct k_sem ept_bound_sem;
	/** Callback function for received data */
	zbus_proxy_agent_recv_cb_t recv_cb;
	/** pointer to proxy agent config, used as user data for the receive callback */
	const struct zbus_proxy_agent *recv_cb_config_ptr;
	/** 1 once backend init completes successfully, otherwise 0 */
	uint8_t initialized;
};

/**
 * @brief Configuration structure for the IPC backend of a Zbus proxy agent.
 */
struct zbus_proxy_agent_ipc_config {
	/** IPC device */
	const struct device *dev;
	/** Endpoint name for IPC */
	const char *ept_name;
	/** Pointer to mutable IPC backend runtime data */
	struct zbus_proxy_agent_ipc_data *data;
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Backend config creation for IPC type
 *
 * @param _name Name of the backend config instance
 * @param _backend_dt_node Device tree node for the backend configuration
 */
#define _ZBUS_PROXY_AGENT_BACKEND_CONFIG_DEFINE_ZBUS_PROXY_AGENT_BACKEND_IPC(_name,                \
									     _backend_dt_node)     \
	static struct zbus_proxy_agent_ipc_data _name##_ipc_data = {                               \
		.ept_cfg = {                                                                       \
			.name = #_name "_ipc_ept",                                                 \
		},                                                                                 \
	};                                                                                         \
	static const struct zbus_proxy_agent_ipc_config _name##_ipc_config = {                     \
		.dev = DEVICE_DT_GET(_backend_dt_node),                                            \
		.ept_name = #_name "_ipc_ept",                                                     \
		.data = &_name##_ipc_data,                                                         \
	};

/**
 * @brief Backend config retrieval for IPC type
 *
 * @param _name Name of the backend config instance
 * @return Pointer to the backend config structure for the specified instance
 */
#define _ZBUS_PROXY_AGENT_GET_BACKEND_CONFIG_ZBUS_PROXY_AGENT_BACKEND_IPC(_name)                   \
	(&_name##_ipc_config)

/* Forward declaration of the IPC backend API */
extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;

/**
 * @brief Backend API retrieval for IPC type
 *
 * @return Pointer to the IPC backend API structure
 */
#define _ZBUS_PROXY_AGENT_GET_BACKEND_API_ZBUS_PROXY_AGENT_BACKEND_IPC                             \
	(&zbus_proxy_agent_ipc_backend_api)

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_IPC_H_ */
