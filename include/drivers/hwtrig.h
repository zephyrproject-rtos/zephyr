/*
 * Copyright (c) 2021 Shlomi Vaknin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public HWTRIG Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HWTRIG_H_
#define ZEPHYR_INCLUDE_DRIVERS_HWTRIG_H_

/**
 * @brief HWTRIG Interface
 * @defgroup hwtrig_interface HWTRIG Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/math_extras.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Provides a type to hold HWTRIG configuration flags.
 */
typedef void* hwtrig_flags_t;

/**
 * @typedef hwtrig_enable_t
 * @brief Enable the hardware trigger
 * See @a hwtrig_enable() for argument description
 */
typedef int (*hwtrig_enable_t)(const struct device *dev,
                    hwtrig_flags_t flags);

/** @brief HWTRIG driver API definition. */
__subsystem struct hwtrig_driver_api {
	hwtrig_enable_t enable;
};

/**
 * @brief Enable HWTRIG trigger.
 *
 * This function enables the hardware trigger configuration. What the
 * configuration is should be driver specific.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param flags trigger configuration.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if invalid function parameters were given
 */
__syscall int hwtrig_enable(const struct device *dev, hwtrig_flags_t flags);

static inline int z_impl_hwtrig_enable(const struct device *dev,
			hwtrig_flags_t flags)
{
	const struct hwtrig_driver_api *api = (struct hwtrig_driver_api *)dev->api;

	return api->enable(dev, flags);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/hwtrig.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_HWTRIG_H_ */
