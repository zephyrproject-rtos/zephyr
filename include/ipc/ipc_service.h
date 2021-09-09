/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_SERVICE_IPC_SERVICE_H_
#define ZEPHYR_INCLUDE_IPC_SERVICE_IPC_SERVICE_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC Service API
 * @defgroup ipc_service_api IPC service APIs
 * @{
 */

/** @brief Event callback structure.
 *
 *  It is registered during endpoint registration.
 *  This structure is part of the endpoint configuration.
 */
struct ipc_service_cb {
	/** @brief Bind was successful.
	 *
	 *  @param priv Private user data.
	 */
	void (*bound)(void *priv);

	/** @brief New packet arrived.
	 *
	 *  @param data Pointer to data buffer.
	 *  @param len Length of @a data.
	 *  @param priv Private user data.
	 */
	void (*received)(const void *data, size_t len, void *priv);

	/** @brief An error occurred.
	 *
	 *  @param message Error message.
	 *  @param priv Private user data.
	 */
	void (*error)(const char *message, void *priv);
};

/** @brief Endpoint instance.
 *
 *  Content is not important for user of the API.
 *  It is implemented in a specific backend.
 */
struct ipc_ept;

/** @brief Endpoint configuration structure. */
struct ipc_ept_cfg {

	/** Name of the endpoint. */
	const char *name;

	/** Endpoint priority. If the backend supports priorities. */
	int prio;

	/** Event callback structure. */
	struct ipc_service_cb cb;

	/** Private user data. */
	void *priv;
};

/** @brief Register IPC endpoint.
 *
 *  Registers IPC endpoint to enable communication with a remote device.
 *
 *  The same function registers endpoints for both master and slave devices.
 *
 *  @param ept Endpoint object.
 *  @param cfg Endpoint configuration.
 *
 *  @retval -EIO when no backend is registered.
 *  @retval -EINVAL when pointer to an endpoint or endpoint configuration is invalid.
 *  @retval Other errno codes depending on the implementation of the backend.
 */
int ipc_service_register_endpoint(struct ipc_ept **ept, const struct ipc_ept_cfg *cfg);

/** @brief Send data using given IPC endpoint.
 *
 *  @param ept Registered endpoint by @ref ipc_service_register_endpoint.
 *  @param data Pointer to the buffer to send.
 *  @param len Number of bytes to send.
 *
 *  @retval -EIO when no backend is registered.
 *  @retval Other errno codes depending on the implementation of the backend.
 */
int ipc_service_send(struct ipc_ept *ept, const void *data, size_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_SERVICE_IPC_SERVICE_H_ */
