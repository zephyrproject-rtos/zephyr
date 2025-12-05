/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup wuc_interface
 * @brief Main header file for WUC (Wakeup Controller) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WUC_H_
#define ZEPHYR_INCLUDE_DRIVERS_WUC_H_

/**
 * @brief Wakeup Controller (WUC) Driver APIs
 * @defgroup wuc_interface WUC Driver APIs
 * @since 4.4
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */
typedef int (*wuc_api_enable_wakeup_source)(const struct device *dev, uint32_t source);

typedef int (*wuc_api_disable_wakeup_source)(const struct device *dev, uint32_t source);

typedef int (*wuc_api_check_wakeup_triggered)(const struct device *dev, uint32_t source);

__subsystem struct wuc_driver_api {
	wuc_api_enable_wakeup_source enable_wakeup_source;
	wuc_api_disable_wakeup_source disable_wakeup_source;
	wuc_api_check_wakeup_triggered check_wakeup_triggered;
};
/** @endcond */

/**
 * @brief Enable a wakeup source
 *
 * @param dev Pointer to the WUC device structure
 * @param source Wakeup source identifier
 *
 * @retval 0 If successful
 * @retval -errno Negative errno code on failure
 */
__syscall int wuc_enable_wakeup_source(const struct device *dev, uint32_t source);

static inline int z_impl_wuc_enable_wakeup_source(const struct device *dev, uint32_t source)
{
	return DEVICE_API_GET(wuc, dev)->enable_wakeup_source(dev, source);
}

/**
 * @brief Disable a wakeup source
 *
 * @param dev Pointer to the WUC device structure
 * @param source Wakeup source identifier
 *
 * @retval 0 If successful
 * @retval -errno Negative errno code on failure
 */
__syscall int wuc_disable_wakeup_source(const struct device *dev, uint32_t source);

static inline int z_impl_wuc_disable_wakeup_source(const struct device *dev, uint32_t source)
{
	return DEVICE_API_GET(wuc, dev)->disable_wakeup_source(dev, source);
}

/**
 * @brief Check if a wakeup source triggered
 *
 * @param dev Pointer to the WUC device structure
 * @param source Wakeup source identifier
 *
 * @retval 1 If wakeup was triggered by this source
 * @retval 0 If wakeup was not triggered by this source
 * @retval -errno Negative errno code on failure
 */
__syscall int wuc_check_wakeup_triggered(const struct device *dev, uint32_t source);

static inline int z_impl_wuc_check_wakeup_triggered(const struct device *dev, uint32_t source)
{
	return DEVICE_API_GET(wuc, dev)->check_wakeup_triggered(dev, source);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/wuc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_WUC_H_ */
