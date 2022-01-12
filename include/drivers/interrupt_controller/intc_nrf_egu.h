/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @file
 * @brief Public APIs for EGU drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_NRF_EGU_H_
#define ZEPHYR_INCLUDE_DRIVERS_NRF_EGU_H_

#include <errno.h>
#include <stddef.h>

#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef egu_channel_cb_t
 * @brief Define the application callback function signature for
 * @ref egu_channel_callback_set function.
 *
 * @param dev     EGU device structure.
 * @param channel EGU channel number that triggered the callback.
 * @param ctx     EGU channel user context.
 */
typedef void (*egu_channel_cb_t)(const struct device *dev, int channel, void *ctx);

/** @brief Driver API structure. */
struct egu_driver_api {
	int (*channel_callback_set)(const struct device *dev,
				    int channel,
				    egu_channel_cb_t cb,
				    void *ctx);
	int (*channel_callback_clear)(const struct device *dev, int channel);
	int (*channel_task_trigger)(const struct device *dev, int channel);
	int (*channel_alloc)(const struct device *dev);
	int (*channel_free)(const struct device *dev, int channel);
};

/**
 * @brief Set channel event handler function.
 *
 * @param dev     EGU device structure.
 * @param channel EGU channel number.
 * @param cb      Pointer to callback function.
 * @param ctx     EGU channel user context.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
static inline int egu_channel_callback_set(const struct device *dev,
					   int channel,
					   egu_channel_cb_t cb,
					   void *ctx)
{
	const struct egu_driver_api *api = (const struct egu_driver_api *)dev->api;

	return api->channel_callback_set(dev, channel, cb, ctx);
}

/**
 * @brief Clear channel event handler function.
 *
 * @param dev     EGU device structure.
 * @param channel EGU channel number.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
static inline int egu_channel_callback_clear(const struct device *dev, int channel)
{
	const struct egu_driver_api *api = (const struct egu_driver_api *)dev->api;

	return api->channel_callback_clear(dev, channel);
}

/**
 * @brief Trigger channel event.
 *
 * @param dev     EGU device structure.
 * @param channel EGU channel number.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
static inline int egu_channel_task_trigger(const struct device *dev, int channel)
{
	const struct egu_driver_api *api = (const struct egu_driver_api *)dev->api;

	return api->channel_task_trigger(dev, channel);
}

/**
 * @brief Allocate EGU channel.
 *
 * @param dev EGU device structure.
 *
 * @retval Allocated channel number if successful, negative errno code otherwise.
 */
static inline int egu_channel_alloc(const struct device *dev)
{
	const struct egu_driver_api *api = (const struct egu_driver_api *)dev->api;

	return api->channel_alloc(dev);
}

/**
 * @brief Free EGU channel.
 *
 * @param dev     EGU device structure.
 * @param channel EGU channel number.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
static inline int egu_channel_free(const struct device *dev, int channel)
{
	const struct egu_driver_api *api = (const struct egu_driver_api *)dev->api;

	return api->channel_free(dev, channel);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_NRF_EGU_H_ */
