/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the IPC service log backend.
 * @ingroup log_backend_ipc_service
 */

#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_IPC_SERVICE_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_IPC_SERVICE_H_

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_backend.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup log_backend_ipc_service IPC service log backend
 * @ingroup log_backend
 * @brief Logging backend that forwards messages over IPC service.
 * @{
 */

/**
 * @brief Runtime state of an IPC service logging backend instance.
 */
struct log_backend_ipc_service_data {
	struct ipc_ept ept; /**< Registered IPC service endpoint. */
	const struct log_backend *log_backend; /**< Backend instance that owns this data. */
	struct k_sem rdy_sem; /**< Released when the endpoint is bound and ready. */
	struct k_work_delayable dropped_work; /**< Deferred work for drop notifications. */
	atomic_t dropped; /**< Dropped messages accumulated since the last notification. */
	int status; /**< Unused. */
	bool panic; /**< @c true after panic mode is entered on this backend. */
	bool ready; /**< @c true once bound and the READY message was sent. */
};

/**
 * @brief Static configuration of an IPC service logging backend instance.
 */
struct log_backend_ipc_service_config {
	const struct device *ipc_instance; /**< IPC service instance device. */
	struct log_backend_ipc_service_data *data; /**< Runtime state for this instance. */
	struct ipc_ept_cfg ept_cfg; /**< Endpoint configuration. */
};

/**
 * @brief IPC service endpoint bound callback.
 *
 * @param priv Pointer to @ref log_backend_ipc_service_config (endpoint private data).
 */
void log_backend_ipc_service_bound_cb(void *priv);

/**
 * @brief IPC service endpoint receive callback.
 *
 * Handles control requests from the link backend, including source name lookup,
 * compile-time and runtime log level queries, and runtime level changes.
 *
 * @param data Pointer to the received @ref log_ipc_service_msg.
 * @param len  Length of @a data, in bytes.
 * @param priv Pointer to @ref log_backend_ipc_service_config (endpoint private data).
 */
void log_backend_ipc_service_recv_cb(const void *data, size_t len, void *priv);

/** @brief Logger backend API implemented by the IPC service backend. */
extern const struct log_backend_api log_backend_ipc_service_api;

/**
 * @brief Create an IPC service logging backend instance.
 *
 * @param _name    Backend symbol name. Used as domain name.
 * @param ipc_node Devicetree node identifier for the IPC service instance
 *                 (for example @c DT_NODELABEL(ipc0)).
 */
#define LOG_BACKEND_IPC_SERVICE_DEFINE(_name, ipc_node) \
	static struct log_backend_ipc_service_data _name##_data; \
	static const struct log_backend_ipc_service_config _name##_config; \
	static const struct log_backend_ipc_service_config _name##_config = { \
		.ipc_instance = DEVICE_DT_GET(ipc_node), \
		.data = &_name##_data, \
		.ept_cfg = { \
			.name = "logging", \
			.prio = 0, \
			.cb = { \
				.bound = log_backend_ipc_service_bound_cb, \
				.received = log_backend_ipc_service_recv_cb, \
			}, \
			.priv = (void *)&_name##_config, \
		}, \
	}; \
	LOG_BACKEND_DEFINE(_name, log_backend_ipc_service_api, true, (void *)&_name##_config)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_IPC_SERVICE_H_ */
