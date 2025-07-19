/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_MSG_SERVICE_IPC_MSG_SERVICE_BACKEND_H_
#define ZEPHYR_INCLUDE_IPC_MSG_SERVICE_IPC_MSG_SERVICE_BACKEND_H_

#include <zephyr/kernel.h>

#include <zephyr/ipc/ipc_msg_service.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC Message Service Backend
 * @defgroup ipc_msg_service_backend IPC Message Service Backend
 * @ingroup ipc_msg_service_api
 * @{
 */

/**
 * @brief IPC message service backend configuration structure.
 *
 * This structure is used for configuration backend during registration.
 */
struct ipc_msg_service_backend {
	/**
	 * @brief Pointer to the function that will be used to open an instance
	 *
	 * @param[in] instance Instance pointer.
	 *
	 * @retval -EALREADY when the instance is already opened.
	 *
	 * @retval 0 on success
	 * @retval other errno codes depending on the implementation of the backend.
	 */
	int (*open_instance)(const struct device *instance);

	/**
	 * @brief Pointer to the function that will be used to close an instance
	 *
	 * @param[in] instance Instance pointer.
	 *
	 * @retval -EALREADY when the instance is not already inited.
	 *
	 * @retval 0 on success
	 * @retval other errno codes depending on the implementation of the backend.
	 */
	int (*close_instance)(const struct device *instance);

	/**
	 * @brief Pointer to the function that will be used to query backend status.
	 *
	 * @param[in]  instance Instance pointer.
	 * @param[in]  token Backend-specific token.
	 * @param[in]  query_type Query type.
	 * @param[in]  query_data Pointer to the query data.
	 * @param[out] query_response Pointer to the query response.
	 *
	 * @retval int Depends on query type. Negative values are errors.
	 *             Positive values are query specific.
	 */
	int (*query)(const struct device *instance, void *token, uint16_t query_type,
		     const void *query_data, void *query_response);

	/**
	 * @brief Pointer to the function that will be used to send data to the endpoint.
	 *
	 * @param[in] instance Instance pointer.
	 * @param[in] token Backend-specific token.
	 * @param[in] msg_type Message type.
	 * @param[in] msg_data Pointer to the message to be sent.
	 *
	 * @retval -EINVAL when instance is invalid.
	 * @retval -ENOENT when the endpoint is not registered with the instance.
	 * @retval -EBADMSG when the message is invalid.
	 * @retval -EBUSY when the instance is busy or not ready.
	 *
	 * @retval 0 if message is sent.
	 * @retval other errno codes depending on the implementation of the backend.
	 */
	int (*send)(const struct device *instance, void *token, uint16_t msg_type,
		    const void *msg_data);

	/**
	 * @brief Pointer to the function that will be used to register endpoints.
	 *
	 * @param[in] instance Instance to register the endpoint onto.
	 * @param[out] token Backend-specific token.
	 * @param[in] cfg Endpoint configuration.
	 *
	 * @retval -EINVAL when the endpoint configuration or instance is invalid.
	 * @retval -EBUSY when the instance is busy or not ready.
	 *
	 * @retval 0 on success
	 * @retval other errno codes depending on the implementation of the backend.
	 */
	int (*register_endpoint)(const struct device *instance, void **token,
				 const struct ipc_msg_ept_cfg *cfg);

	/**
	 * @brief Pointer to the function that will be used to deregister endpoints.
	 *
	 * @param[in] instance Instance from which to deregister the endpoint.
	 * @param[in] token Backend-specific token.
	 *
	 * @retval -EINVAL when the endpoint configuration or instance is invalid.
	 * @retval -ENOENT when the endpoint is not registered with the instance.
	 * @retval -EBUSY when the instance is busy or not ready.
	 *
	 * @retval 0 on success
	 * @retval other errno codes depending on the implementation of the backend.
	 */
	int (*deregister_endpoint)(const struct device *instance, void *token);
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_MSG_SERVICE_IPC_MSG_SERVICE_BACKEND_H_ */
