/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_IPC_SERVICE_H_
#define ZEPHYR_INCLUDE_IPC_IPC_SERVICE_H_

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC
 * @defgroup ipc IPC
 * @{
 * @}
 */

/**
 * @brief IPC Service API
 * @defgroup ipc_service_api IPC service APIs
 * @ingroup ipc
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

/**
 * Some terminology:
 *
 * - INSTANCE: an instance is the external representation of a physical
 *             communication channel between two domains / CPUs.
 *
 *             The actual implementation and internal representation of the
 *             instance is peculiar to each backend. For example for
 *             OpenAMP-based backends, an instance is usually represented by a
 *             shared memory region and a couple of IPM devices for RX/TX
 *             signalling.
 *
 *             It's important to note that an instance per se is not used to
 *             send data between domains / CPUs. To send and receive data the
 *             user have to create (register) an endpoint in the instance
 *             connecting the two domains of interest.
 *
 *             It's possible to have zero or multiple endpoints in one single
 *             instance, each one used to exchange data, possibly with different
 *             priorities.
 *
 *             The creation of the instances is left to the backend (usually at
 *             init time), while the registration of the endpoints is left to
 *             the user (usually at run time).
 *
 * - ENDPOINT: an endpoint is the entity the user must use to send / receive
 *             data between two domains (connected by the instance). An
 *             endpoint is always associated to an instance.
 *
 * - BACKEND: the backend must take care of at least two different things:
 *
 *            1) creating the instances at init time
 *            2) creating / registering the endpoints onto an instance at run
 *               time when requested by the user
 *
 *            The API doesn't mandate a way for the backend to create the
 *            instances but itis strongly recommended to use the DT to retrieve
 *            the configuration parameters for the instance.
 *
 * Common API usage from the application prospective:
 *
 *   HOST                                         REMOTE
 *   -----------------------------------------------------------------------------
 *   # Open the (same) instance on host and remote
 *   ipc_service_open()                           ipc_service_open()
 *
 *   # Register the endpoints
 *   ipc_service_register_endpoint()              ipc_service_register_endpoint()
 *   .bound()                                     .bound()
 *
 *   # After the .bound() callbacks are received the communication channel
 *   # is ready to be used
 *
 *   # Start sending and receiving data
 *   ipc_service_send()
 *                                                .receive()
 *                                                ipc_service_send()
 *   .receive()
 *
 *
 * Common API usage from the application prospective when using NOCOPY feature:
 *
 *   HOST                                         REMOTE
 *   -----------------------------------------------------------------------------
 *   ipc_service_open()                           ipc_service_open()
 *
 *   ipc_service_register_endpoint()              ipc_service_register_endpoint()
 *   .bound()                                     .bound()
 *
 *   # Get a pointer to an available TX buffer
 *   ipc_service_get_tx_buffer()
 *
 *   # Fill the buffer with data
 *
 *   # Send out the buffer
 *   ipc_service_send_nocopy()
 *                                                .receive()
 *
 *                                                # Get hold of the received RX buffer
 *                                                # in the .receive callback
 *                                                ipc_service_hold_rx_buffer()
 *
 *                                                # Copy the data out of the buffer at
 *                                                # user convenience
 *
 *                                                # Release the buffer when done
 *                                                ipc_service_release_rx_buffer()
 *
 *    # Get another TX buffer
 *    ipc_service_get_tx_buffer()
 *
 *    # We can also drop it if needed
 *    ipc_service_drop_tx_buffer()
 *
 */

/**
 * @endcond
 */

/** @brief Event callback structure.
 *
 *  It is registered during endpoint registration.
 *  This structure is part of the endpoint configuration.
 */
struct ipc_service_cb {
	/** @brief Bind was successful.
	 *
	 *  This callback is called when the endpoint binding is successful.
	 *
	 *  @param[in] priv Private user data.
	 */
	void (*bound)(void *priv);

	/** @brief New packet arrived.
	 *
	 *  This callback is called when new data is received.
	 *
	 *  @note When @ref ipc_service_hold_rx_buffer is not used, the data
	 *	  buffer is to be considered released and available again only
	 *	  when this callback returns.
	 *
	 *  @param[in] data Pointer to data buffer.
	 *  @param[in] len Length of @a data.
	 *  @param[in] priv Private user data.
	 */
	void (*received)(const void *data, size_t len, void *priv);

	/** @brief An error occurred.
	 *
	 *  @param[in] message Error message.
	 *  @param[in] priv Private user data.
	 */
	void (*error)(const char *message, void *priv);
};

/** @brief Endpoint instance.
 *
 *  Token is not important for user of the API. It is implemented in a
 *  specific backend.
 */
struct ipc_ept {

	/** Instance this endpoint belongs to. */
	const struct device *instance;

	/** Backend-specific token used to identify an endpoint in an instance. */
	void *token;
};

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

/** @brief Open an instance
 *
 *  Function to be used to open an instance before being able to register a new
 *  endpoint on it.
 *
 *  @param[in] instance Instance to open.
 *
 *  @retval -EINVAL when instance configuration is invalid.
 *  @retval -EIO when no backend is registered.
 *  @retval -EALREADY when the instance is already opened (or being opened).
 *
 *  @retval 0 on success or when not implemented on the backend (not needed).
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_open_instance(const struct device *instance);

/** @brief Close an instance
 *
 *  Function to be used to close an instance. All endpoints must be
 *  deregistered using ipc_service_deregister_endpoint before this
 *  is called.
 *
 *  @param[in] instance Instance to close.
 *
 *  @retval -EINVAL when instance configuration is invalid.
 *  @retval -EIO when no backend is registered.
 *  @retval -EALREADY when the instance is not already opened.
 *  @retval -EBUSY when an endpoint exists that hasn't been
 *           deregistered
 *
 *  @retval 0 on success or when not implemented on the backend (not needed).
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_close_instance(const struct device *instance);

/** @brief Register IPC endpoint onto an instance.
 *
 *  Registers IPC endpoint onto an instance to enable communication with a
 *  remote device.
 *
 *  The same function registers endpoints for both host and remote devices.
 *
 *  @param[in] instance Instance to register the endpoint onto.
 *  @param[in] ept Endpoint object.
 *  @param[in] cfg Endpoint configuration.
 *
 *  @note Keep the variable pointed by @p cfg alive when endpoint is in use.
 *
 *  @retval -EIO when no backend is registered.
 *  @retval -EINVAL when instance, endpoint or configuration is invalid.
 *  @retval -EBUSY when the instance is busy.
 *
 *  @retval 0 on success.
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_register_endpoint(const struct device *instance,
				  struct ipc_ept *ept,
				  const struct ipc_ept_cfg *cfg);

/** @brief Deregister an IPC endpoint from its instance.
 *
 *  Deregisters an IPC endpoint from its instance.
 *
 *  The same function deregisters endpoints for both host and remote devices.
 *
 *  @param[in] ept Endpoint object.
 *
 *  @retval -EIO when no backend is registered.
 *  @retval -EINVAL when instance, endpoint or configuration is invalid.
 *  @retval -ENOENT when the endpoint is not registered with the instance.
 *  @retval -EBUSY when the instance is busy.
 *
 *  @retval 0 on success.
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_deregister_endpoint(struct ipc_ept *ept);

/** @brief Send data using given IPC endpoint.
 *
 *  @param[in] ept Registered endpoint by @ref ipc_service_register_endpoint.
 *  @param[in] data Pointer to the buffer to send.
 *  @param[in] len Number of bytes to send.
 *
 *  @retval -EIO when no backend is registered or send hook is missing from
 *               backend.
 *  @retval -EINVAL when instance or endpoint is invalid.
 *  @retval -ENOENT when the endpoint is not registered with the instance.
 *  @retval -EBADMSG when the data is invalid (i.e. invalid data format,
 *		     invalid length, ...)
 *  @retval -EBUSY when the instance is busy.
 *  @retval -ENOMEM when no memory / buffers are available.
 *
 *  @retval bytes number of bytes sent.
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_send(struct ipc_ept *ept, const void *data, size_t len);

/** @brief Get the TX buffer size
 *
 *  Get the maximal size of a buffer which can be obtained by @ref
 *  ipc_service_get_tx_buffer
 *
 *  @param[in] ept Registered endpoint by @ref ipc_service_register_endpoint.
 *
 *  @retval -EIO when no backend is registered or send hook is missing from
 *		 backend.
 *  @retval -EINVAL when instance or endpoint is invalid.
 *  @retval -ENOENT when the endpoint is not registered with the instance.
 *  @retval -ENOTSUP when the operation is not supported by backend.
 *
 *  @retval size TX buffer size on success.
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_get_tx_buffer_size(struct ipc_ept *ept);

/** @brief Get an empty TX buffer to be sent using @ref ipc_service_send_nocopy
 *
 *  This function can be called to get an empty TX buffer so that the
 *  application can directly put its data into the sending buffer without copy
 *  from an application buffer.
 *
 *  It is the application responsibility to correctly fill the allocated TX
 *  buffer with data and passing correct parameters to @ref
 *  ipc_service_send_nocopy function to perform data no-copy-send mechanism.
 *
 *  The size parameter can be used to request a buffer with a certain size:
 *  - if the size can be accommodated the function returns no errors and the
 *    buffer is allocated
 *  - if the requested size is too big, the function returns -ENOMEM and the
 *    the buffer is not allocated.
 *  - if the requested size is '0' the buffer is allocated with the maximum
 *    allowed size.
 *
 *  In all the cases on return the size parameter contains the maximum size for
 *  the returned buffer.
 *
 *  When the function returns no errors, the buffer is intended as allocated
 *  and it is released under two conditions: (1) when sending the buffer using
 *  @ref ipc_service_send_nocopy (and in this case the buffer is automatically
 *  released by the backend), (2) when using @ref ipc_service_drop_tx_buffer on
 *  a buffer not sent.
 *
 *  @param[in] ept Registered endpoint by @ref ipc_service_register_endpoint.
 *  @param[out] data Pointer to the empty TX buffer.
 *  @param[in,out] size Pointer to store the requested TX buffer size. If the
 *			function returns -ENOMEM, this parameter returns the
 *			maximum allowed size.
 *  @param[in] wait Timeout waiting for an available TX buffer.
 *
 *  @retval -EIO when no backend is registered or send hook is missing from
 *		 backend.
 *  @retval -EINVAL when instance or endpoint is invalid.
 *  @retval -ENOENT when the endpoint is not registered with the instance.
 *  @retval -ENOTSUP when the operation or the timeout is not supported by backend.
 *  @retval -ENOBUFS when there are no TX buffers available.
 *  @retval -EALREADY when a buffer was already claimed and not yet released.
 *  @retval -ENOMEM when the requested size is too big (and the size parameter
 *		    contains the maximum allowed size).
 *
 *  @retval 0 on success.
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_get_tx_buffer(struct ipc_ept *ept, void **data, uint32_t *size, k_timeout_t wait);

/** @brief Drop and release a TX buffer
 *
 *  Drop and release a TX buffer. It is possible to drop only TX buffers
 *  obtained by using @ref ipc_service_get_tx_buffer.
 *
 *  @param[in] ept Registered endpoint by @ref ipc_service_register_endpoint.
 *  @param[in] data Pointer to the TX buffer.
 *
 *  @retval -EIO when no backend is registered or send hook is missing from
 *		 backend.
 *  @retval -EINVAL when instance or endpoint is invalid.
 *  @retval -ENOENT when the endpoint is not registered with the instance.
 *  @retval -ENOTSUP when this is not supported by backend.
 *  @retval -EALREADY when the buffer was already dropped.
 *  @retval -ENXIO when the buffer was not obtained using @ref
 *		   ipc_service_get_tx_buffer
 *
 *  @retval 0 on success.
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_drop_tx_buffer(struct ipc_ept *ept, const void *data);

/** @brief Send data in a TX buffer reserved by @ref ipc_service_get_tx_buffer
 *         using the given IPC endpoint.
 *
 *  This is equivalent to @ref ipc_service_send but in this case the TX buffer
 *  has been obtained by using @ref ipc_service_get_tx_buffer.
 *
 *  The application has to take the responsibility for getting the TX buffer
 *  using @ref ipc_service_get_tx_buffer and filling the TX buffer with the data.
 *
 *  After the @ref ipc_service_send_nocopy function is issued the TX buffer is
 *  no more owned by the sending task and must not be touched anymore unless
 *  the function fails and returns an error.
 *
 *  If this function returns an error, @ref ipc_service_drop_tx_buffer can be
 *  used to drop the TX buffer.
 *
 *  @param[in] ept Registered endpoint by @ref ipc_service_register_endpoint.
 *  @param[in] data Pointer to the buffer to send obtained by @ref
 *		    ipc_service_get_tx_buffer.
 *  @param[in] len Number of bytes to send.
 *
 *  @retval -EIO when no backend is registered or send hook is missing from
 *		 backend.
 *  @retval -EINVAL when instance or endpoint is invalid.
 *  @retval -ENOENT when the endpoint is not registered with the instance.
 *  @retval -EBADMSG when the data is invalid (i.e. invalid data format,
 *		     invalid length, ...)
 *  @retval -EBUSY when the instance is busy.
 *
 *  @retval bytes number of bytes sent.
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_send_nocopy(struct ipc_ept *ept, const void *data, size_t len);

/** @brief Holds the RX buffer for usage outside the receive callback.
 *
 *  Calling this function prevents the receive buffer from being released
 *  back to the pool of shmem buffers. This function can be called in the
 *  receive callback when the user does not want to copy the message out in
 *  the callback itself.
 *
 *  After the message is processed, the application must release the buffer
 *  using the @ref ipc_service_release_rx_buffer function.
 *
 *  @param[in] ept Registered endpoint by @ref ipc_service_register_endpoint.
 *  @param[in] data Pointer to the RX buffer to hold.
 *
 *  @retval -EIO when no backend is registered or release hook is missing from
 *		 backend.
 *  @retval -EINVAL when instance or endpoint is invalid.
 *  @retval -ENOENT when the endpoint is not registered with the instance.
 *  @retval -EALREADY when the buffer data has been hold already.
 *  @retval -ENOTSUP when this is not supported by backend.
 *
 *  @retval 0 on success.
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_hold_rx_buffer(struct ipc_ept *ept, void *data);

/** @brief Release the RX buffer for future reuse.
 *
 *  When supported by the backend, this function can be called after the
 *  received message has been processed and the buffer can be marked as
 *  reusable again.
 *
 *  It is possible to release only RX buffers on which @ref
 *  ipc_service_hold_rx_buffer was previously used.
 *
 *  @param[in] ept Registered endpoint by @ref ipc_service_register_endpoint.
 *  @param[in] data Pointer to the RX buffer to release.
 *
 *  @retval -EIO when no backend is registered or release hook is missing from
 *		 backend.
 *  @retval -EINVAL when instance or endpoint is invalid.
 *  @retval -ENOENT when the endpoint is not registered with the instance.
 *  @retval -EALREADY when the buffer data has been already released.
 *  @retval -ENOTSUP when this is not supported by backend.
 *  @retval -ENXIO when the buffer was not hold before using @ref
 *		   ipc_service_hold_rx_buffer
 *
 *  @retval 0 on success.
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_release_rx_buffer(struct ipc_ept *ept, void *data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_IPC_SERVICE_H_ */
