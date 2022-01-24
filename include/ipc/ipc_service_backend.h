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
	/** @brief Pointer to the function that will be used to open an instance
	 *
	 *  @param instance Instance pointer.
	 *
	 *  @retval -EALREADY when the instance is already opened.
	 *
	 *  @retval 0 on success
	 *  @retval other errno codes depending on the implementation of the
	 *	    backend.
	 */
	int (*open_instance)(const struct device *instance);

	/** @brief Pointer to the function that will be used to send data to the endpoint.
	 *
	 *  @param instance Instance pointer.
	 *  @param token Backend-specific token.
	 *  @param data Pointer to the buffer to send.
	 *  @param len Number of bytes to send.
	 *
	 *  @retval -EINVAL when instance is invalid.
	 *  @retval -EBADMSG when the message is invalid.
	 *  @retval -EBUSY when the instance is busy or not ready.
	 *
	 *  @retval 0 on success
	 *  @retval other errno codes depending on the implementation of the
	 *	    backend.
	 */
	int (*send)(const struct device *instance, void *token,
		    const void *data, size_t len);

	/** @brief Pointer to the function that will be used to register endpoints.
	 *
	 *  @param instance Instance to register the endpoint onto.
	 *  @param token Backend-specific token.
	 *  @param cfg Endpoint configuration.
	 *
	 *  @retval -EINVAL when the endpoint configuration or instance is invalid.
	 *  @retval -EBUSY when the instance is busy or not ready.
	 *
	 *  @retval 0 on success
	 *  @retval other errno codes depending on the implementation of the
	 *	    backend.
	 */
	int (*register_endpoint)(const struct device *instance,
				 void **token,
				 const struct ipc_ept_cfg *cfg);
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_SERVICE_IPC_SERVICE_BACKEND_H_ */
