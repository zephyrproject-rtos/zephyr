/*
 * Copyright (c) 2022 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SATELLITE_H_
#define ZEPHYR_INCLUDE_DRIVERS_SATELLITE_H_

/**
 * @file
 * @brief Public Satellite APIs
 * @defgroup satellite_api Satellite APIs
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Satellite configuration.
 */
struct satellite_modem_config {
	/** Frequency in Hz to use for transceiving */
	uint32_t frequency;

	/** TX-power in mW to use for transmission */
	int16_t tx_power;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

/**
 * @typedef satellite_api_send_result_cb()
 * @brief Callback API for to be return with TX status
 */
typedef void (*satellite_api_send_result_cb)(bool status);

/**
 * @typedef satellite_api_config()
 * @brief Callback API for configuring the satellite module
 *
 * @see satellite_config() for argument descriptions.
 */
typedef int (*satellite_api_config)(const struct device *dev,
			       struct satellite_modem_config *config);

/**
 * @typedef satellite_api_send_sync()
 * @brief Callback API for sending data over satellite
 *
 * @see satellite_send_sync() for argument descriptions.
 */
typedef int (*satellite_api_send_sync)(const struct device *dev,
			     uint8_t *data, uint32_t data_len);

/**
 * @typedef satellite_api_send_pool_async()
 * @brief Callback API for sending pool of data asynchronously over satellite
 * with delay between each transmission
 *
 * @see satellite_send_pool_async() for argument descriptions.
 */
typedef int (*satellite_api_send_pool_async)(const struct device *dev,
				   uint8_t *data, uint32_t data_len,
				   uint8_t number_of_send,
				   k_timeout_t time_between_send,
				   satellite_api_send_result_cb *result_cb);


struct satellite_driver_api {
	satellite_api_config config;
	satellite_api_send_sync send_sync;
	satellite_api_send_pool_async send_pool_async;
};

/** @endcond */

/**
 * @brief Configure the satellite modem
 *
 * @param dev     Satellite device
 * @param config  Data structure containing the intended configuration for the
		  modem
 * @return 0 on success, negative on error
 */
static inline int satellite_config(const struct device *dev,
			      struct satellite_modem_config *config)
{
	const struct satellite_driver_api *api =
		(const struct satellite_driver_api *)dev->api;

	return api->config(dev, config);
}

/**
 * @brief Send data over satellite
 *
 * @note This blocks until transmission is complete.
 * @warning On success, it informs modem has successfully sent data
 *       over driver but is not an acknowledge (RX is not available)
 *
 * @param dev       Satellite device
 * @param data      Data to be sent
 * @param data_len  Length of the data to be sent
 * @return 0 on success, negative on error
 */
static inline int satellite_send_sync(const struct device *dev,
			    uint8_t *data, uint32_t data_len)
{
	const struct satellite_driver_api *api =
		(const struct satellite_driver_api *)dev->api;

	return api->send_sync(dev, data, data_len);
}

/**
 * @brief Asynchronously send (pool of) data over satellite
 *
 * @note This returns immediately after starting transmission, and locks
 *       the satellite modem until the transmission completes. Once
 *       tranmission done, result_cb is called with operation result
 * @warning Result callback informs modem has successfully sent data
 *       over modem but is not an acknowledge (RX is not available)
 *
 * @param dev       Satellite device
 * @param data      Data to be sent
 * @param data_len  Length of the data to be sent
 * @param number_of_send Number of time message shall be sent
 * @param time_between_send Time delay between each transmission
 * @param result_cb Callback to be called when pool of send done
 * @return 0 on success, negative on error
 */
static inline int satellite_send_pool_async(const struct device *dev,
							 uint8_t *data, uint32_t data_len,
							 uint8_t number_of_send,
							 k_timeout_t time_between_send,
							 satellite_api_send_result_cb *result_cb)
{
	const struct satellite_driver_api *api =
		(const struct satellite_driver_api *)dev->api;

	return api->send_pool_async(dev, data, data_len, number_of_send,
				time_between_send, result_cb);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif	/* ZEPHYR_INCLUDE_DRIVERS_SATELLITE_H_ */
