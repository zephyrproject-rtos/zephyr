/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_IPC_SERVICE_H_
#define ZEPHYR_INCLUDE_IPC_IPC_SERVICE_H_

#include <stdio.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC Service API
 * @defgroup ipc_service_api IPC service APIs
 * @{
 *
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
 *  @retval -EINVAL when instance configuration is invalid.
 *  @retval -EIO when no backend is registered.
 *  @retval -EALREADY when the instance is already opened (or being opened).
 *
 *  @retval 0 on success or when not implemented on the backend (not needed).
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_open_instance(const struct device *instance);


/** @brief Register IPC endpoint onto an instance.
 *
 *  Registers IPC endpoint onto an instance to enable communication with a
 *  remote device.
 *
 *  The same function registers endpoints for both host and remote devices.
 *
 *  @param instance Instance to register the endpoint onto.
 *  @param ept Endpoint object.
 *  @param cfg Endpoint configuration.
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

/** @brief Send data using given IPC endpoint.
 *
 *  @param ept Registered endpoint by @ref ipc_service_register_endpoint.
 *  @param data Pointer to the buffer to send.
 *  @param len Number of bytes to send.
 *
 *  @retval -EIO when no backend is registered or send hook is missing from
 *	    backend.
 *  @retval -EINVAL when instance or endpoint is invalid.
 *  @retval -EBADMSG when the message is invalid.
 *  @retval -EBUSY when the instance is busy.
 *
 *  @retval 0 on success.
 *  @retval other errno codes depending on the implementation of the backend.
 */
int ipc_service_send(struct ipc_ept *ept, const void *data, size_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_IPC_SERVICE_H_ */
