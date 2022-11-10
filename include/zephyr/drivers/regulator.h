/*
 * Copyright 2019-2020 Peter Bigot Consulting, LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_REGULATOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_REGULATOR_H_

/**
 * @brief Regulator Interface
 * @defgroup regulator_interface Regulator Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/sys/onoff.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** @brief Driver-specific API functions to support regulator control. */
__subsystem struct regulator_driver_api {
	int (*enable)(const struct device *dev, struct onoff_client *cli);
	int (*disable)(const struct device *dev);
};

/** @endcond */

/**
 * @brief Enable a regulator.
 *
 * Reference-counted request that a regulator be turned on. This is an
 * asynchronous operation; if successfully initiated the result will be
 * communicated through the @p cli parameter.
 *
 * A regulator is considered "on" when it has reached a stable/usable state.
 *
 * @funcprops \isr_ok \pre_kernel_ok
 *
 * @param dev Regulator device instance
 * @param cli On-off client instance. This is used to notify the caller when the
 * attempt to turn on the regulator has completed (can be `NULL`).
 *
 * @retval 0 If enable request has been successfully initiated.
 * @retval -errno Negative errno in case of failure (can be from onoff_request()
 * or individual regulator drivers).
 */
static inline int regulator_enable(const struct device *dev,
				   struct onoff_client *cli)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	return api->enable(dev, cli);
}

/**
 * @brief Disable a regulator.
 *
 * Release a regulator after a previous regulator_enable() completed
 * successfully.
 *
 * If the release removes the last dependency on the regulator it will begin a
 * transition to its "off" state. There is currently no mechanism to notify when
 * the regulator has completely turned off.
 *
 * This must be invoked at most once for each successful regulator_enable().
 *
 * @funcprops \isr_ok
 *
 * @param dev Regulator device instance.
 *
 * @retval 0 If enable request has been successfully initiated.
 * @retval -errno Negative errno in case of failure (can be from onoff_release()
 * or individual regulator drivers).
 */
static inline int regulator_disable(const struct device *dev)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)dev->api;

	return api->disable(dev);
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_REGULATOR_H_ */
