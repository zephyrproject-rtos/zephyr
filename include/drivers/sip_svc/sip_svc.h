/*
 * Copyright (c) 2021, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SIP_SVC_H_
#define ZEPHYR_INCLUDE_DRIVERS_SIP_SVC_H_

/**
 * @file
 * @brief Public API for Arm SiP services
 *
 * Arm SiP services driver provides the capability to send the
 * SMC/HVC call from kernel to hypervisor/secure monitor firmware
 * running at EL2/EL3.
 *
 * Only allow one SMC and one HVC driver per system.
 *
 * The driver support multiple clients.
 *
 * The client must open a channel before sending any request and
 * close the channel immediately after complete. The driver only
 * allow one channel at one time.
 *
 * The driver will return the SMC/HVC return value to the client
 * via callback function.
 *
 * The client state machine
 * - IDLE  : Initial state.
 * - OPEN  : The client will switch from IDLE to OPEN once it
 *           successfully open the channel. On the other hand, it
 *           will switch from OPEN to IDLE state once it successfully
 *           close the channel.
 * - ABORT : The client has closed the channel, however, there are
 *           incomplete transactions being left over. The driver
 *           will only move the client back to IDLE state once all
 *           transactions completed. The client is not allowed to
 *           re-open the channel when in ABORT state/
 */

#include <drivers/sip_svc/sip_svc_ll.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief SiP Services driver API.
 */
struct sip_svc_driver_api {
	sip_svc_register_fn		reg;
	sip_svc_unregister_fn		unreg;
	sip_svc_open_fn			open;
	sip_svc_close_fn		close;
	sip_svc_send_fn			send;
	sip_svc_get_priv_data_fn	get_priv_data;
	sip_svc_print_info_fn		print_info;
};

/**
 * @brief Register a client on Arm SiP services driver
 *
 * On success, the client will be at IDLE state in the driver and
 * the driver will return a token to the client. The client can then
 * use the token to open the channel on the driver and communicate
 * with hypervisor/secure monitor firmware running at EL2/EL3.
 *
 * @param dev Device structure whose driver provides Arm SMC/HVC
 *            SiP services.
 * @param priv_data Pointer to client private data.
 * @return token on success, 0xFFFFFFFF on failure.
 */
static inline uint32_t sip_svc_register(const struct device *dev,
					void *priv_data)
{
	if (!device_is_ready(dev)) {
		return SIP_SVC_ID_INVALID;
	}

	const struct sip_svc_driver_api *api =
		(const struct sip_svc_driver_api *)dev->api;

	return api->reg(dev->data, priv_data);
}

/**
 * @brief Unregister a client on Arm SiP services driver
 *
 * On success, detach the client from the driver. Unregistration
 * is only allowed when all transactions belong to the client are closed.
 *
 * @param dev Device structure whose driver provides Arm SiP services.
 * @param c_token Client's token
 * @return 0 on success, negative errno on failure.
 */
static inline int sip_svc_unregister(const struct device *dev,
				      uint32_t c_token)
{
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	const struct sip_svc_driver_api *api =
		(const struct sip_svc_driver_api *)dev->api;

	return api->unreg(dev->data, c_token);
}

/**
 * @brief Client requests to open a channel on Arm SiP services driver.
 *
 * Client must open a channel on the driver before sending any request via
 * SMC/HVC to hypervisor/secure monitor firmware running at EL2/EL3.
 *
 * The driver only allow one opened channel at one time and it is protected
 * by mutex.
 *
 * @param dev Device structure whose driver provides Arm SiP services.
 * @param c_token Client's token
 * @param timeout_us Waiting time if the mutex have been locked.
 *                   When the mutex have been locked:
 *                   - returns non-zero error code immediately if value is 0
 *                   - wait forever if the value is 0xFFFFFFFF
 *                   - otherwise, for the given time
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int sip_svc_open(const struct device *dev,
			       uint32_t c_token,
			       uint32_t timeout_us)
{
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	const struct sip_svc_driver_api *api =
		(const struct sip_svc_driver_api *)dev->api;

	return api->open(dev->data, c_token, timeout_us);
}

/**
 * @brief Client requests to close the channel on Arm SiP services driver.
 *
 * Client must close the channel immediately once complete.
 *
 * @param dev Device structure whose driver provides Arm SiP services.
 * @param c_token Client's token
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int sip_svc_close(const struct device *dev,
			       uint32_t c_token)
{
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	const struct sip_svc_driver_api *api =
		(const struct sip_svc_driver_api *)dev->api;

	return api->close(dev->data, c_token);
}

/**
 * @brief Client requests to send a SMC/HVC call to EL3/EL2
 *
 * Client must open a channel on the device before using this function.
 * This function is non-blocking and can be called from any context. The
 * driver will return a Transaction ID to the client if the request
 * is being accepted. Client callback is called when the transaction is
 * completed.
 *
 * @param dev Device structure whose driver provides Arm SiP services.
 * @param c_token Client's token
 * @param req Address to the user input in struct sip_svc_request format.
 * @param size Data size in 'req' address.
 * @param cb Callback. SMC/SVC return value will be passed to client via
 *           context in struct sip_svc_response format in callback.
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int sip_svc_send(const struct device *dev,
			       uint32_t c_token,
			       char *req,
			       int size,
			       sip_svc_cb_fn cb)
{
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	const struct sip_svc_driver_api *api =
		(const struct sip_svc_driver_api *)dev->api;

	return api->send(dev->data, c_token, req, size, cb);
}

/**
 * @brief Get the address pointer to the client private data.
 *
 * The pointer is provided by client during registration.
 *
 * @param dev Device structure whose driver provides Arm SiP services.
 * @param c_token Client's token
 *
 * @return address pointer to the client private data
 */
static inline void *sip_svc_get_priv_data(const struct device *dev,
			       uint32_t c_token)
{
	if (!device_is_ready(dev)) {
		return NULL;
	}

	const struct sip_svc_driver_api *api =
		(const struct sip_svc_driver_api *)dev->api;

	return api->get_priv_data(dev->data, c_token);
}

/**
 * @brief Print the Arm SiP services driver information
 *
 * Print the driver's channel, transaction statistics and client
 * information.
 *
 * @param dev Device structure whose driver provides Arm SiP services.
 *
 * @return void.
 */
static inline void sip_svc_print_info(const struct device *dev)
{
	if (!device_is_ready(dev)) {
		return;
	}

	const struct sip_svc_driver_api *api =
		(const struct sip_svc_driver_api *)dev->api;

	api->print_info(dev->data);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SIP_SVC_H_ */
