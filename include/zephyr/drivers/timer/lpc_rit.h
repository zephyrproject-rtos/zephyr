/*
 * Copyright (c) 2024 VCI Development
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_TIMER_LPC_RIT_H_
#define ZEPHYR_INCLUDE_DRIVERS_TIMER_LPC_RIT_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RIT timer callback function signature
 *
 * @param dev RIT device
 * @param user_data User data passed to callback
 */
typedef void (*rit_timer_callback_t)(const struct device *dev, void *user_data);

/**
 * @brief RIT timer configuration
 */
struct rit_timer_cfg {
	/** Timer period in ticks */
	uint64_t period;
	/** Timer callback function */
	rit_timer_callback_t callback;
	/** User data for callback */
	void *user_data;
	/** Enable auto-clear mode */
	bool auto_clear;
	/** Run timer in debug mode */
	bool run_in_debug;
};

/**
 * @typedef rit_api_configure_t
 * @brief API function to configure RIT timer
 */
typedef int (*rit_api_configure_t)(const struct device *dev,
				    const struct rit_timer_cfg *cfg);

/**
 * @typedef rit_api_start_t
 * @brief API function to start RIT timer
 */
typedef int (*rit_api_start_t)(const struct device *dev);

/**
 * @typedef rit_api_stop_t
 * @brief API function to stop RIT timer
 */
typedef int (*rit_api_stop_t)(const struct device *dev);

/**
 * @typedef rit_api_get_value_t
 * @brief API function to get current timer value
 */
typedef int (*rit_api_get_value_t)(const struct device *dev, uint64_t *value);

/**
 * @typedef rit_api_set_mask_t
 * @brief API function to set timer mask value
 */
typedef int (*rit_api_set_mask_t)(const struct device *dev, uint64_t mask);

/**
 * @typedef rit_api_get_frequency_t
 * @brief API function to get timer frequency
 */
typedef uint32_t (*rit_api_get_frequency_t)(const struct device *dev);

/**
 * @brief RIT driver API
 */
__subsystem struct rit_driver_api {
	rit_api_configure_t configure;
	rit_api_start_t start;
	rit_api_stop_t stop;
	rit_api_get_value_t get_value;
	rit_api_set_mask_t set_mask;
	rit_api_get_frequency_t get_frequency;
};

/**
 * @brief Configure RIT timer
 *
 * @param dev RIT device
 * @param cfg Timer configuration
 * @return 0 on success, negative errno on failure
 */
__syscall int rit_configure(const struct device *dev,
			     const struct rit_timer_cfg *cfg);

static inline int z_impl_rit_configure(const struct device *dev,
				       const struct rit_timer_cfg *cfg)
{
	const struct rit_driver_api *api = dev->api;

	return api->configure(dev, cfg);
}

/**
 * @brief Start RIT timer
 *
 * @param dev RIT device
 * @return 0 on success, negative errno on failure
 */
__syscall int rit_start(const struct device *dev);

static inline int z_impl_rit_start(const struct device *dev)
{
	const struct rit_driver_api *api = dev->api;

	return api->start(dev);
}

/**
 * @brief Stop RIT timer
 *
 * @param dev RIT device
 * @return 0 on success, negative errno on failure
 */
__syscall int rit_stop(const struct device *dev);

static inline int z_impl_rit_stop(const struct device *dev)
{
	const struct rit_driver_api *api = dev->api;

	return api->stop(dev);
}

/**
 * @brief Get current timer value
 *
 * @param dev RIT device
 * @param value Pointer to store current value
 * @return 0 on success, negative errno on failure
 */
__syscall int rit_get_value(const struct device *dev, uint64_t *value);

static inline int z_impl_rit_get_value(const struct device *dev, uint64_t *value)
{
	const struct rit_driver_api *api = dev->api;

	return api->get_value(dev, value);
}

/**
 * @brief Set timer mask value
 *
 * @param dev RIT device
 * @param mask Mask value
 * @return 0 on success, negative errno on failure
 */
__syscall int rit_set_mask(const struct device *dev, uint64_t mask);

static inline int z_impl_rit_set_mask(const struct device *dev, uint64_t mask)
{
	const struct rit_driver_api *api = dev->api;

	return api->set_mask(dev, mask);
}

/**
 * @brief Get timer frequency
 *
 * @param dev RIT device
 * @return Timer frequency in Hz
 */
__syscall uint32_t rit_get_frequency(const struct device *dev);

static inline uint32_t z_impl_rit_get_frequency(const struct device *dev)
{
	const struct rit_driver_api *api = dev->api;

	return api->get_frequency(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_TIMER_LPC_RIT_H_ */