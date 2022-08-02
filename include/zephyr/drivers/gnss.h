/**
 * @file gnss.h
 *
 * @brief Public GNSS API.
 *
 */

/*
 * Copyright (c) 2022, Trackunit A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GNSS_H_
#define ZEPHYR_INCLUDE_DRIVERS_GNSS_H_

/**
 * @brief GNSS Interface
 * @defgroup gnss_interface GNSS Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/location.h>
#include <zephyr/data/rtc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*gnss_datetime_get_t)(const struct device *dev,
			uint32_t fix_interval fix_interval);

typedef int (*gnss_fix_rate_set_t)(const struct device *dev,
			uint32_t fix_interval fix_interval);

typedef int (*gnss_fix_rate_get_t)(const struct device *dev,
			uint32_t fix_interval *fix_interval);

enum gnss_power_mode {
	GNSS_POWER_MODE_LOW,
	GNSS_POWER_MODE_BALANCED,
	GNSS_POWER_MODE_HIGH,
};

typedef int (*gnss_power_mode_set_t)(const struct device *dev,
			enum gnss_power_mode power_mode);

typedef int (*gnss_power_mode_get_t)(const struct device *dev,
			enum gnss_power_mode *power_mode);

enum gnss_pps_mode {
	GNSS_PPS_MODE_DISABLED,
	GNSS_PPS_MODE_ENABLED
	GNSS_PPS_MODE_ENABLED_LOCKED,
};

typedef int (*gnss_pps_mode_set)(const struct device *dev,
			enum gnss_pps_mode pps_config);

typedef int (*gnss_pps_mode_get)(const struct device *dev,
			enum gnss_pps_mode *pps_config);

typedef int (*gnss_satelites_cnt_get)(const struct device *dev,
			uint32_t satelites_cnt);

/**
 * @brief GNSS API
 *
 * @details The GNSS API expands upon the location API
 *
 * @note Both the location and GNSS APIs can be used for
 * devices which implement this API
 *
 */
__subsystem struct gnss_driver_api {
	struct location_api location_api;
	gnss_fix_rate_set_t fix_rate_set;
	gnss_fix_rate_get_t fix_rate_get;
	gnss_power_mode_set_t power_mode_set;
	gnss_power_mode_get_t power_mode_get;
	gnss_pps_mode pps_mode_set;
	gnss_pps_mode pps_mode_get;
	gnss_satelites_cnt_get satelites_cnt_get;
};

/**
 * @brief Get the latest position from the device instance
 *
 * @param dev Device instance
 * @param position Destination for position data
 * @return -EINVAL if any argument is NULL
 * @return -ENOSYS if api call is not implemented by driver
 * @return -ENOENT if no data is available
 * @return 0 if successful, negative errno code otherwise
 */
static inline int gnss_position_get(const struct device *dev, struct location_position *position)
{
	return location_position_get(dev, position);
}

/**
 * @brief Get the latest bearing from the device instance
 *
 * @param dev Device instance
 * @param bearing Destination for bearing data
 * @return -EINVAL if any argument is NULL
 * @return -ENOSYS if api call is not implemented by driver
 * @return -ENOENT if no data is available
 * @return 0 if successful, negative errno code otherwise
 */
static inline int gnss_bearing_get(const struct device *dev, struct location_bearing *bearing)
{
	return location_bearing_get(dev, bearing);
}

/**
 * @brief Get the latest speed from the device instance
 *
 * @param dev Device instance
 * @param speed Destination for speed data
 * @return -EINVAL if any argument is NULL
 * @return -ENOSYS if api call is not implemented by driver
 * @return -ENOENT if no data is available
 * @return 0 if successful, negative errno code otherwise
 */
static inline int gnss_speed_get(const struct device *dev, struct location_speed *speed)
{
	location_speed_get(dev, speed);
}

/**
 * @brief Get the latest altitude from the device instance
 *
 * @param dev Device instance
 * @param altitude Destination for altitude data
 * @return -EINVAL if any argument is NULL
 * @return -ENOSYS if api call is not implemented by driver
 * @return -ENOENT if no data is available
 * @return 0 if successful, negative errno code otherwise
 */
static inline int gnss_altitude_get(const struct device *dev, struct location_altitude *altitude)
{
	location_altitude_get(dev, altitude);
}

/**
 * @brief Set the location event handler
 *
 * @param dev Device instance
 * @param handler The handler to invoke when a location event occurs, can be set to NULL
 * @param user_data User data provided to handler when it is invoked
 * @return -EINVAL if device is NULL
 * @return -ENOSYS if api call is not implemented by driver
 * @return 0 if successful, negative errno code otherwise
 */
static inline int gnss_event_handler_set(const struct device *dev,
			location_event_handler_t handler, void *user_data);
{
	return location_event_handler_set(dev, handler, user_data);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/gnss.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_GNSS_H_ */
