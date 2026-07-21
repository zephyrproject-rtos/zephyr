/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header file for fan driver API.
 * @ingroup fan_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FAN_H_
#define ZEPHYR_INCLUDE_DRIVERS_FAN_H_

/**
 * @brief Interfaces for fans
 * @defgroup fan_interface Fan
 * @since 4.5
 * @version 0.1.0
 * @ingroup io_interfaces
 *
 * Speed-control abstraction for cooling fans and similar variable-speed
 * blowers.
 *
 * Fan speed is expressed as a percentage of the fan's full range, from 0
 * (stopped) to @ref FAN_SPEED_MAX (full speed). Backends able to measure
 * the rotation rate expose it through fan_get_rpm().
 *
 * @{
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum speed level, range is 0 to FAN_SPEED_MAX. */
#define FAN_SPEED_MAX 100U

/**
 * @def_driverbackendgroup{Fan, fan_interface}
 * @{
 */

/**
 * @brief Set the fan speed.
 *
 * See fan_set_speed() for argument descriptions.
 */
typedef int (*fan_set_speed_t)(const struct device *dev, uint8_t percent);

/**
 * @brief Get the currently configured fan speed.
 *
 * See fan_get_speed() for argument descriptions.
 */
typedef int (*fan_get_speed_t)(const struct device *dev, uint8_t *percent);

/**
 * @brief Read the measured fan rotation rate.
 *
 * See fan_get_rpm() for argument descriptions.
 */
typedef int (*fan_get_rpm_t)(const struct device *dev, uint32_t *rpm);

/**
 * @driver_ops{Fan}
 */
__subsystem struct fan_driver_api {
	/** @driver_ops_mandatory @copybrief fan_set_speed */
	fan_set_speed_t set_speed;
	/** @driver_ops_mandatory @copybrief fan_get_speed */
	fan_get_speed_t get_speed;
	/** @driver_ops_optional @copybrief fan_get_rpm */
	fan_get_rpm_t get_rpm;
};

/** @} */

/**
 * @brief Set the fan speed.
 *
 * Speed is expressed as a percentage between 0 and @ref FAN_SPEED_MAX,
 * where 0 stops the fan and FAN_SPEED_MAX runs it at full speed. The
 * setting persists until changed.
 *
 * @param dev      Fan device.
 * @param percent  Desired speed, 0 to @ref FAN_SPEED_MAX inclusive.
 *
 * @retval 0        Speed updated successfully.
 * @retval -EINVAL  @p percent is out of range.
 * @retval -errno   Negative errno code on failure.
 */
static inline int fan_set_speed(const struct device *dev, uint8_t percent)
{
	return DEVICE_API_GET(fan, dev)->set_speed(dev, percent);
}

/**
 * @brief Get the currently configured fan speed.
 *
 * Returns the last speed requested through fan_set_speed(), expressed as
 * a percentage between 0 and @ref FAN_SPEED_MAX. This reflects the
 * commanded value, not a measured rotation rate; use fan_get_rpm() for
 * the latter.
 *
 * @param dev      Fan device.
 * @param percent  Destination for the configured speed.
 *
 * @retval 0       Speed read successfully.
 * @retval -errno  Negative errno code on failure.
 */
static inline int fan_get_speed(const struct device *dev, uint8_t *percent)
{
	return DEVICE_API_GET(fan, dev)->get_speed(dev, percent);
}

/**
 * @brief Read the measured fan rotation rate.
 *
 * Retrieves the fan's rotation rate in revolutions per minute from a
 * tachometer or equivalent feedback signal.
 *
 * @param dev  Fan device.
 * @param rpm  Destination for the measured rotation rate in RPM.
 *
 * @retval 0        Rotation rate read successfully.
 * @retval -ENOSYS  The backend cannot measure the rotation rate.
 * @retval -errno   Negative errno code on failure.
 */
static inline int fan_get_rpm(const struct device *dev, uint32_t *rpm)
{
	const struct fan_driver_api *api = DEVICE_API_GET(fan, dev);

	if (api->get_rpm == NULL) {
		return -ENOSYS;
	}

	return api->get_rpm(dev, rpm);
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_FAN_H_ */
