/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_IPC_H_
#define ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_IPC_H_

#include <zephyr/zbus/zbus.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/zbus/multidomain/zbus_multidomain_types.h>

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
 * @brief Structure for IPC backend configuration.
 */
struct zbus_multidomain_ipc_config {
	/** IPC device */
	const struct device *dev;

	/** IPC endpoint */
	struct ipc_ept ipc_ept;

	/** IPC endpoint configuration */
	struct ipc_ept_cfg *ept_cfg;

	/** Semaphore to signal when the IPC endpoint is bound */
	struct k_sem ept_bound_sem;

	/** Callback function for received messages */
	int (*recv_cb)(const struct zbus_proxy_agent_msg *msg);

	/** Callback function for ACKs */
	int (*ack_cb)(uint32_t msg_id, void *user_data);

	/** User data for the ACK callback */
	void *ack_cb_user_data;

	/** Work item for sending ACKs */
	struct k_work ack_work;

	/** Message ID to ACK */
	uint32_t ack_msg_id;
};

/** @cond INTERNAL_HIDDEN */

/* IPC backend API structure */
extern const struct zbus_proxy_agent_api zbus_multidomain_ipc_api;

/**
 * @brief Macros to get the API and configuration for the IPC backend.
 *
 * These macros are used to retrieve the API and configuration for the IPC backend
 * of the proxy agent. The macros are used in "zbus_multidomain.h" to define the
 * backend specific configurations and API for the IPC type of proxy agent.
 *
 * @param _name The name of the proxy agent.
 */
#define _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC()         &zbus_multidomain_ipc_api
#define _ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(_name) (void *)&_name##_ipc_config

/**
 * @brief Macros to generate device specific backend configurations for the IPC type.
 *
 * This macro generates the backend specific configurations for the IPC type of
 * proxy agent. The macro is used in "zbus_multidomain.h" to create the
 * backend specific configurations for the IPC type of proxy agent.
 *
 * @param _name The name of the proxy agent.
 * @param _nodeid The device node ID for the proxy agent.
 */
#define _ZBUS_GENERATE_BACKEND_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(_name, _nodeid)                    \
	static struct ipc_ept_cfg _name##_ipc_ept_cfg = {                                          \
		.name = "ipc_ept_" #_name,                                                         \
	};                                                                                         \
	static struct zbus_multidomain_ipc_config _name##_ipc_config = {                           \
		.dev = DEVICE_DT_GET(_nodeid),                                                     \
		.ept_cfg = &_name##_ipc_ept_cfg,                                                   \
	}

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_IPC_H_ */
