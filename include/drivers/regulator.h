/*
 * Copyright 2019-2020 Peter Bigot Consulting, LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API for voltage and current regulators.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_REGULATOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_REGULATOR_H_

/**
 * @brief Regulator Interface
 * @defgroup regulator_interface Regulator Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <drivers/gpio.h>
#include <sys/onoff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Driver-specific API functions to support regulator control.
 */
__subsystem struct regulator_driver_api {
	int (*enable)(const struct device *dev, struct onoff_client *cli);
	int (*disable)(const struct device *dev);
};

/**
 * @brief Enable a regulator.
 *
 * Reference-counted request that a regulator be turned on.  This is
 * an asynchronous operation; if successfully initiated the result
 * will be communicated through the @p cli parameter.
 *
 * A regulator is considered "on" when it has reached a stable/usable
 * state.
 *
 * @note This function is *isr-ok* and *pre-kernel-ok*.
 *
 * @param reg a regulator device
 *
 * @param cli used to notify the caller when the attempt to turn on
 * the regulator has completed.
 *
 * @return non-negative on successful initiation of the request.
 * Negative values indicate failures from onoff_request() or
 * individual regulator drivers.
 */
static inline int regulator_enable(const struct device *reg,
				   struct onoff_client *cli)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)reg->api;

	return api->enable(reg, cli);
}

/**
 * @brief Disable a regulator.
 *
 * Release a regulator after a previous regulator_enable() completed
 * successfully.
 *
 * If the release removes the last dependency on the regulator it will
 * begin a transition to its "off" state.  There is currently no
 * mechanism to notify when the regulator has completely turned off.
 *
 * This must be invoked at most once for each successful
 * regulator_enable().
 *
 * @note This function is *isr-ok*.
 *
 * @param reg a regulator device
 *
 * @return non-negative on successful completion of the release
 * request.  Negative values indicate failures from onoff_release() or
 * individual regulator drivers.
 */
static inline int regulator_disable(const struct device *reg)
{
	const struct regulator_driver_api *api =
		(const struct regulator_driver_api *)reg->api;

	return api->disable(reg);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_REGULATOR_H_ */
