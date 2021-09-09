/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_SERVICE_IPC_SERVICE_BACKEND_H_
#define ZEPHYR_INCLUDE_IPC_SERVICE_IPC_SERVICE_BACKEND_H_

#include <ipc/ipc_service.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC Service backend
 * @ingroup ipc_service_api
 * @{
 */

/** @brief IPC backend configuration structure.
 *
 *  This structure is used for configuration backend during registration.
 */
struct ipc_service_backend {
	/** @brief Name of the IPC backend. */
	const char *name;

	/** @brief Pointer to the function that will be used to send data to the endpoint.
	 *
	 *  @param ept Registered endpoint.
	 *  @param data Pointer to the buffer to send.
	 *  @param len Number of bytes to send.
	 *
	 *  @retval Status code.
	 */
	int (*send)(struct ipc_ept *ept, const void *data, size_t len);

	/** @brief Pointer to the function that will be used to register endpoints.
	 *
	 *  @param ept Endpoint object.
	 *  @param cfg Endpoint configuration.
	 *
	 *  @retval Status code.
	 */
	int (*register_endpoint)(struct ipc_ept **ept, const struct ipc_ept_cfg *cfg);
};

/** @brief IPC backend registration.
 *
 *  Registration must be done before using IPC Service.
 *
 *  @param backend Configuration of the backend.
 *
 *  @retval -EALREADY The backend is already registered.
 *  @retval -EINVAL The backend configuration is incorrect.
 *  @retval Zero on success.
 *
 */
int ipc_service_register_backend(const struct ipc_service_backend *backend);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_SERVICE_IPC_SERVICE_BACKEND_H_ */
