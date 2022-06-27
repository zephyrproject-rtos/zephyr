/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RPMSG_SERVICE_RPMSG_SERVICE_H_
#define ZEPHYR_INCLUDE_RPMSG_SERVICE_RPMSG_SERVICE_H_

#include <openamp/open_amp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RPMsg service API
 * @defgroup rpmsg_service_api RPMsg service APIs
 * @{
 */

/**
 * @brief Register IPC endpoint
 *
 * Registers IPC endpoint to enable communication with a remote device.
 * The endpoint is created when the slave device registers it.
 *
 * The same function registers endpoints for both master and slave devices.
 *
 * @param name String containing the name of the endpoint. Must be identical
 *             for master and slave
 * @param cb Callback executed when data are available on given endpoint
 *
 * @retval >=0 id of registered endpoint on success;
 * @retval -EINPROGRESS when requested to register an endpoint after endpoints
 *         creation procedure has started;
 * @retval -ENOMEM when there is not enough slots to register the endpoint;
 * @retval <0 an other negative errno code, reported by rpmsg.
 */
int rpmsg_service_register_endpoint(const char *name, rpmsg_ept_cb cb);

/**
 * @brief Send data using given IPC endpoint
 *
 * @param endpoint_id Id of registered endpoint, obtained by
 *                    @ref rpmsg_service_register_endpoint
 * @param data Pointer to the buffer to send through RPMsg service
 * @param len Number of bytes to send.
 *
 * @retval >=0 number of sent bytes;
 * @retval <0 an error code, reported by rpmsg.
 */
int rpmsg_service_send(int endpoint_id, const void *data, size_t len);

/**
 * @brief Check if endpoint is bound.
 *
 * Checks if remote endpoint has been created
 * and the master has bound its endpoint to it.
 *
 * @param endpoint_id Id of registered endpoint, obtained by
 *                    @ref rpmsg_service_register_endpoint
 *
 * @retval true endpoint is bound
 * @retval false endpoint not bound
 */
bool rpmsg_service_endpoint_is_bound(int endpoint_id);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RPMSG_SERVICE_RPMSG_SERVICE_H_ */
