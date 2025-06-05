/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_IPC_MSG_SERVICE_H_
#define ZEPHYR_INCLUDE_IPC_IPC_MSG_SERVICE_H_

#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/ipc/ipc_msg_evts.h>
#include <zephyr/ipc/ipc_msg_queries.h>
#include <zephyr/ipc/ipc_msg_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC Message Service API
 * @defgroup ipc_msg_service_api IPC Message Service APIs
 * @ingroup ipc
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in public documentation.
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
 *   ipc_msg_service_open()                       ipc_msg_service_open()
 *
 *   # Register the endpoints
 *   ipc_msg_service_register_endpoint()          ipc_msg_service_register_endpoint()
 *   .bound()                                     .bound()
 *
 *   # After the .bound() callbacks are received the communication channel
 *   # is ready to be used
 *
 *   # Start sending and receiving data
 *   ipc_msg_service_send()
 *                                                .receive()
 *                                                ipc_msg_service_send()
 *   .receive()
 *
 */

/**
 * @endcond
 */

/**
 * @name IPC Message Service Send Flags
 * @anchor ipc_msg_svc_send_flags
 *
 * There are 16 bits (IPC_MSG_SEND_FLAG_CUSTOM_BIT_*) which can be used to
 * define backend specific flags.
 *
 * @{
 */

/**
 * Wait for remote acknowledgment before returning from @ref ipc_msg_service_send.
 */
#define IPC_MSG_SEND_FLAG_SYNC BIT(0)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_0 BIT(16)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_1 BIT(17)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_2 BIT(18)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_3 BIT(19)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_4 BIT(20)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_5 BIT(21)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_6 BIT(22)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_7 BIT(23)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_8 BIT(24)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_9 BIT(25)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_10 BIT(26)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_11 BIT(27)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_12 BIT(28)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_13 BIT(29)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_14 BIT(30)

/** Custom flag bit for backends. */
#define IPC_MSG_SEND_FLAG_CUSTOM_BIT_15 BIT(31)

/** @} */

/**
 * @brief Event callback structure.
 *
 * It is registered during endpoint registration.
 * This structure is part of the endpoint configuration.
 */
struct ipc_msg_service_cb {
	/**
	 * @brief Bind was successful.
	 *
	 * This callback is called when the endpoint binding is successful.
	 *
	 * @param[in] priv Private data.
	 */
	void (*bound)(void *priv);

	/**
	 * @brief The endpoint unbound by the remote.
	 *
	 * This callback is called when the endpoint binding is removed. It may happen on
	 * different reasons, e.g. when the remote deregistered the endpoint, connection was
	 * lost, or remote CPU got reset.
	 *
	 * You may want to do some cleanup, resetting, e.t.c. and after that if you want to bound
	 * again, you can register the endpoint. When the remote becomes available again and it
	 * also registers the endpoint, the binding will be reestablished and the `bound()`
	 * callback will be called.
	 *
	 * @param[in] priv Private data.
	 */
	void (*unbound)(void *priv);

	/**
	 * @brief New packet arrived.
	 *
	 * This callback is called when new data is received.
	 *
	 * @note When this function returns, the data is to be considered released and
	 *       is no longer valid.
	 *
	 * @param[in] msg_type Message type of the received message.
	 * @param[in] msg_data Pointer to message.
	 * @param[in] priv Private data.
	 *
	 * @retval 0 if received message is processed successfully.
	 * @retval other errno code if error. Positive values are backend specific.
	 */
	int (*received)(uint8_t msg_type, const void *msg_data, void *priv);

	/**
	 * @brief Event triggered.
	 *
	 * This callback is called when certain events are triggered.
	 *
	 * @note When this function returns, the data is to be considered released and
	 *       is no longer valid.
	 *
	 * @param[in] evt_type Event type.
	 * @param[in] evt_data Pointer to event related data.
	 * @param[in] priv Private data.
	 *
	 * @retval 0 if event is acknowledged or processed successfully.
	 * @retval other errno code if error. Positive values are backend specific.
	 */
	int (*event)(uint8_t evt_type, const void *evt_data, void *priv);

	/**
	 * @brief An error occurred.
	 *
	 * @param[in] message Error message.
	 * @param[in] priv Private data.
	 */
	void (*error)(const char *message, void *priv);
};

/**
 * @brief Endpoint instance.
 */
struct ipc_msg_ept {

	/** Instance this endpoint belongs to. */
	const struct device *instance;

	/**
	 * Backend-specific token used to identify an endpoint in an instance.
	 *
	 * @note Only used by specific backend and should not be used by user of the API.
	 */
	void *token;
};

/**
 * @brief Endpoint configuration structure.
 */
struct ipc_msg_ept_cfg {

	/** Name of the endpoint. */
	const char *name;

	/** Endpoint priority. If the backend supports priorities. */
	int prio;

	/** Event callback structure. */
	struct ipc_msg_service_cb cb;

	/** Private data. */
	void *priv;
};

/**
 * @brief Open an instance
 *
 * Function to be used to open an instance before being able to register a new
 * endpoint on it.
 *
 * @param[in] instance Instance to open.
 *
 * @retval -EINVAL when instance configuration is invalid.
 * @retval -EIO when no backend is registered.
 * @retval -EALREADY when the instance is already opened (or being opened).
 *
 * @retval 0 on success or when not implemented on the backend (not needed).
 * @retval other errno codes depending on the implementation of the backend.
 */
int ipc_msg_service_open_instance(const struct device *instance);

/**
 * @brief Close an instance
 *
 * Function to be used to close an instance. All bounded endpoints must be
 * deregistered using ipc_service_deregister_endpoint before this
 * is called.
 *
 * @param[in] instance Instance to close.
 *
 * @retval -EINVAL when instance configuration is invalid.
 * @retval -EIO when no backend is registered.
 * @retval -EALREADY when the instance is not already opened.
 * @retval -EBUSY when an endpoint exists that hasn't been
 *          deregistered
 *
 * @retval 0 on success or when not implemented on the backend (not needed).
 * @retval other errno codes depending on the implementation of the backend.
 */
int ipc_msg_service_close_instance(const struct device *instance);

/**
 * @brief Register IPC endpoint onto an instance.
 *
 * Registers IPC endpoint onto an instance to enable communication with a
 * remote device.
 *
 * The same function registers endpoints for both host and remote devices.
 *
 * @param[in] instance Instance to register the endpoint onto.
 * @param[in] ept Endpoint object.
 * @param[in] cfg Endpoint configuration.
 *
 * @note Keep the variable pointed by @p cfg alive when endpoint is in use.
 *
 * @retval -EIO when no backend is registered.
 * @retval -EINVAL when instance, endpoint or configuration is invalid.
 * @retval -EBUSY when the instance is busy.
 *
 * @retval 0 on success.
 * @retval other errno codes depending on the implementation of the backend.
 */
int ipc_msg_service_register_endpoint(const struct device *instance, struct ipc_msg_ept *ept,
				      const struct ipc_msg_ept_cfg *cfg);

/**
 * @brief Deregister an IPC endpoint from its instance.
 *
 * Deregisters an IPC endpoint from its instance.
 *
 * The same function deregisters endpoints for both host and remote devices.
 *
 * @param[in] ept Endpoint object.
 *
 * @retval -EIO when no backend is registered.
 * @retval -EINVAL when instance, endpoint or configuration is invalid.
 * @retval -ENOENT when the endpoint is not registered with the instance.
 * @retval -EBUSY when the instance is busy.
 *
 * @retval 0 on success.
 * @retval other errno codes depending on the implementation of the backend.
 */
int ipc_msg_service_deregister_endpoint(struct ipc_msg_ept *ept);

/**
 * @brief Send data using given IPC endpoint.
 *
 * @param[in] ept Registered endpoint by @ref ipc_service_register_endpoint.
 * @param[in] msg_type Message type.
 * @param[in] msg_data Pointer to the message to be sent.
 *
 * @retval -EIO when no backend is registered or send hook is missing from
 *              backend.
 * @retval -EINVAL when instance or endpoint is invalid.
 * @retval -ENOENT when the endpoint is not registered with the instance.
 * @retval -EBADMSG when the message is invalid.
 * @retval -EBUSY when the instance is busy.
 * @retval -ENOTSUP when message type is not supported.
 *
 * @retval 0 if message is sent.
 * @retval other errno codes depending on the implementation of the backend.
 */
int ipc_msg_service_send(struct ipc_msg_ept *ept, uint8_t msg_type, const void *msg_data);

/**
 * @brief Query given IPC endpoint.
 *
 * @param[in] ept Registered endpoint by @ref ipc_service_register_endpoint.
 * @param[in] query_type Query type.
 * @param[in] query_data Pointer to the query data.
 *
 * @retval -EIO when no backend is registered or query hook is missing from
 *              backend.
 * @retval -EINVAL when instance or endpoint is invalid.
 * @retval -ENOENT when the endpoint is not registered with the instance.
 * @retval -ENOTSUP when query type is not supported.
 *
 * @retval other Depends on query type. Negative values are errors.
 *               Positive values are query specific.
 */
int ipc_msg_service_query(struct ipc_msg_ept *ept, uint8_t query_type, const void *query_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_IPC_MSG_SERVICE_H_ */
