/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_SERVICE_IPC_SERVICE_BACKEND_H_
#define ZEPHYR_INCLUDE_IPC_SERVICE_IPC_SERVICE_BACKEND_H_

#include <zephyr/ipc/ipc_service.h>
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
	 *  @param[in] instance Instance pointer.
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
	 *  @param[in] instance Instance pointer.
	 *  @param[in] token Backend-specific token.
	 *  @param[in] data Pointer to the buffer to send.
	 *  @param[in] len Number of bytes to send.
	 *
	 *  @retval -EINVAL when instance is invalid.
	 *  @retval -ENOENT when the endpoint is not registered with the instance.
	 *  @retval -EBADMSG when the message is invalid.
	 *  @retval -EBUSY when the instance is busy or not ready.
	 *  @retval -ENOMEM when no memory / buffers are available.
	 *
	 *  @retval bytes number of bytes sent.
	 *  @retval other errno codes depending on the implementation of the
	 *	    backend.
	 */
	int (*send)(const struct device *instance, void *token,
		    const void *data, size_t len);

	/** @brief Pointer to the function that will be used to register endpoints.
	 *
	 *  @param[in] instance Instance to register the endpoint onto.
	 *  @param[out] token Backend-specific token.
	 *  @param[in] cfg Endpoint configuration.
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

	/** @brief Pointer to the function that will return the TX buffer size
	 *
	 *  @param[in] instance Instance pointer.
	 *  @param[in] token Backend-specific token.
	 *
	 *  @retval -EINVAL when instance is invalid.
	 *  @retval -ENOENT when the endpoint is not registered with the instance.
	 *  @retval -ENOTSUP when the operation is not supported.
	 *
	 *  @retval size TX buffer size on success.
	 *  @retval other errno codes depending on the implementation of the
	 *	    backend.
	 */
	int (*get_tx_buffer_size)(const struct device *instance, void *token);

	/** @brief Pointer to the function that will return an empty TX buffer.
	 *
	 *  @param[in] instance Instance pointer.
	 *  @param[in] token Backend-specific token.
	 *  @param[out] data Pointer to the empty TX buffer.
	 *  @param[in,out] len Pointer to store the TX buffer size.
	 *  @param[in] wait Timeout waiting for an available TX buffer.
	 *
	 *  @retval -EINVAL when instance is invalid.
	 *  @retval -ENOENT when the endpoint is not registered with the instance.
	 *  @retval -ENOTSUP when the operation or the timeout is not supported.
	 *  @retval -ENOBUFS when there are no TX buffers available.
	 *  @retval -EALREADY when a buffer was already claimed and not yet released.
	 *  @retval -ENOMEM when the requested size is too big (and the size parameter
	 *		    contains the maximum allowed size).
	 *
	 *  @retval 0 on success
	 *  @retval other errno codes depending on the implementation of the
	 *	    backend.
	 */
	int (*get_tx_buffer)(const struct device *instance, void *token,
			     void **data, uint32_t *len, k_timeout_t wait);

	/** @brief Pointer to the function that will drop a TX buffer.
	 *
	 *  @param[in] instance Instance pointer.
	 *  @param[in] token Backend-specific token.
	 *  @param[in] data Pointer to the TX buffer.
	 *
	 *  @retval -EINVAL when instance is invalid.
	 *  @retval -ENOENT when the endpoint is not registered with the instance.
	 *  @retval -ENOTSUP when this function is not supported.
	 *  @retval -EALREADY when the buffer was already dropped.
	 *
	 *  @retval 0 on success
	 *  @retval other errno codes depending on the implementation of the
	 *	    backend.
	 */
	int (*drop_tx_buffer)(const struct device *instance, void *token,
			      const void *data);

	/** @brief Pointer to the function that will be used to send data to
	 *	   the endpoint when the TX buffer has been obtained using @ref
	 *	   ipc_service_get_tx_buffer
	 *
	 *  @param[in] instance Instance pointer.
	 *  @param[in] token Backend-specific token.
	 *  @param[in] data Pointer to the buffer to send.
	 *  @param[in] len Number of bytes to send.
	 *
	 *  @retval -EINVAL when instance is invalid.
	 *  @retval -ENOENT when the endpoint is not registered with the instance.
	 *  @retval -EBADMSG when the data is invalid (i.e. invalid data format,
	 *		     invalid length, ...)
	 *  @retval -EBUSY when the instance is busy or not ready.
	 *
	 *  @retval bytes number of bytes sent.
	 *  @retval other errno codes depending on the implementation of the
	 *	    backend.
	 */
	int (*send_nocopy)(const struct device *instance, void *token,
			   const void *data, size_t len);

	/** @brief Pointer to the function that will hold the RX buffer
	 *
	 *  @param[in] instance Instance pointer.
	 *  @param[in] token Backend-specific token.
	 *  @param[in] data Pointer to the RX buffer to hold.
	 *
	 *  @retval -EINVAL when instance is invalid.
	 *  @retval -ENOENT when the endpoint is not registered with the instance.
	 *  @retval -EALREADY when the buffer data has been already hold.
	 *  @retval -ENOTSUP when this function is not supported.
	 *
	 *  @retval 0 on success
	 *  @retval other errno codes depending on the implementation of the
	 *	    backend.
	 */
	int (*hold_rx_buffer)(const struct device *instance, void *token,
			      void *data);

	/** @brief Pointer to the function that will release the RX buffer.
	 *
	 *  @param[in] instance Instance pointer.
	 *  @param[in] token Backend-specific token.
	 *  @param[in] data Pointer to the RX buffer to release.
	 *
	 *  @retval -EINVAL when instance is invalid.
	 *  @retval -ENOENT when the endpoint is not registered with the instance.
	 *  @retval -EALREADY when the buffer data has been already released.
	 *  @retval -ENOTSUP when this function is not supported.
	 *
	 *  @retval 0 on success
	 *  @retval other errno codes depending on the implementation of the
	 *	    backend.
	 */
	int (*release_rx_buffer)(const struct device *instance, void *token,
				 void *data);
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_SERVICE_IPC_SERVICE_BACKEND_H_ */
