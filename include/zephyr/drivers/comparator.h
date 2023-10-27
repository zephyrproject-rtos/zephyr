/*
 * Copyright (c) 2023 Adam Mitchell, Brill Power Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Comparator public API header file.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COMP_H
#define ZEPHYR_INCLUDE_DRIVERS_COMP_H

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief See comp_enable() for argument descriptions
 */
typedef void (*comp_api_enable)(const struct device *dev);

/**
 * @brief See comp_disable() for argument descriptions
 */
typedef void (*comp_api_disable)(const struct device *dev);

#ifdef CONFIG_COMP_LOCK
/**
 * @brief See comp_lock() for argument descriptions
 */
typedef void (*comp_api_lock)(const struct device *dev);
#endif /* CONFIG_COMP_LOCK */

__subsystem struct comp_driver_api
{
	comp_api_enable enable;
	comp_api_disable disable;
#ifdef CONFIG_COMP_LOCK \
	comp_api_lock lock;
#endif /* CONFIG_COMP_LOCK */
};

/**
 * @brief Enable comparator pointed to by dev->config->base
 * @param dev Pointer to the device structure for the driver instance
 * @retval None
 */
static inline void comp_enable(const struct device *dev)
{
	const struct comp_driver_api *api = (const struct comp_driver_api *)dev->api;

	api->enable(dev);
}

/**
 * @brief Disable comparator pointed to by dev->config->base
 * @param dev Pointer to the device structure for the driver instance
 * @retval None
 */
static inline void comp_disable(const struct device *dev)
{
	const struct comp_driver_api *api = (const struct comp_driver_api *)dev->api;

	api->disable(dev);
}

#ifdef CONFIG_COMP_LOCK
/**
 * @brief Prevent runtime configuration of comparator (after init function)
 * @param dev Pointer to the device structure for the driver instance
 * @retval None
 */
static inline void comp_lock(const struct device *dev)
{
	const struct comp_driver_api *api = (const struct comp_driver_api *)dev->api;

	api->lock(dev);
}
#endif /* CONFIG_COMP_LOCK */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMP_H */
