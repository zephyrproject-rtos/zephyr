/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ACTUATOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_ACTUATOR_H_

/**
 * @brief Interface for actuators.
 * @defgroup actuator_interface Actuator interface
 * @since 4.3
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/dsp/types.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

typedef int (*actuator_api_set_setpoint_t)(const struct device *dev, q31_t setpoint);

__subsystem struct actuator_driver_api {
	actuator_api_set_setpoint_t set_setpoint;
};

/** @endcond */

/**
 * @brief Set actuator setpoint
 *
 * @param dev Actuator device
 * @param setpoint Setpoint in Q31 format
 *
 * @retval 0 Successful
 * @retval -errno code Failure
 */
__syscall int actuator_set_setpoint(const struct device *dev, q31_t setpoint);

static inline int z_impl_actuator_set_setpoint(const struct device *dev, q31_t setpoint)
{
	return DEVICE_API_GET(actuator, dev)->set_setpoint(dev, setpoint);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/actuator.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_ACTUATOR_H_ */
