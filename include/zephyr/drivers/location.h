/**
 * @file location.h
 *
 * @brief Public location API.
 *
 */

/*
 * Copyright (c) 2022, Trackunit A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LOCATION_H_
#define ZEPHYR_INCLUDE_DRIVERS_LOCATION_H_

/**
 * @brief Location Interface
 * @defgroup location_interface Location Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Location data
 *
 * The latitude and longitude are in the unit nano degree. 1 nano
 * degree corresponds to approx. 0.1mm along the earths equator and
 * meridians.
 *
 * The accuracy describes the radius within which the real
 * position is with a 68.2% certainty, in millimeters.
 */
struct location_position {
	/** Latitudal position in nanodegrees (0 to +-180000000000) */
	int64_t latitude;
	/** Longitudal position in nanodegrees (0 to +-180000000000) */
	int64_t longitude;
	/** The uptime when the position was determined in ticks */
	k_ticks_t uptime_ticks;
	/** Accuracy radius in millimeters */
	uint32_t accuracy;
};

/**
 * @brief Bearing data
 *
 * The accuracy describes the uncertainty of the bearing in +-
 * millidegrees with a 68.2% certainty.
 */
struct location_bearing {
	/** Bearing angle relative to True North in millidegrees (0 to 360000) */
	uint32_t bearing;
	/** The uptime when the bearing was determined in ticks */
	k_ticks_t uptime_ticks;
	/** Accuracy in millidegrees */
	uint32_t accuracy;
};

/**
 * @brief Speed data
 *
 * The accuracy describes the uncertainty of the speed in +-
 * millimeters per second with a 68.2% certainty.
 */
struct location_speed {
	/** Speed over ground in millimeters per second */
	uint32_t speed;
	/** The uptime when the speed was determined in ticks */
	k_ticks_t uptime_ticks;
	/** Accuracy in millimeters per second */
	uint32_t accuracy;
};

/**
 * @brief Altitude data
 *
 * The accuracy describes the uncertainty of the altitude
 * in +- millimeters with a 68.2% certainty.
 *
 * Note that the reference from which the altitude is determined
 * is specific to the device which produced the data
 */
struct location_altitude {
	/** Altitude in millimeters */
	int32_t altitude;
	/** The uptime when the altitude was determined in ticks */
	k_ticks_t uptime_ticks;
	/** Accuracy in millimeters */
	uint32_t accuracy;
};

/**
 * @typedef location_position_get_t
 * @brief API for getting newest position data
 *
 * See struct location_position for description of position data
 */
typedef int (*location_position_get_t)(const struct device *dev,
			struct location_position *position);

/**
 * @typedef location_bearing_get_t
 * @brief API for getting newest bearing data
 *
 * See struct location_bearing for description of bearing data
 */
typedef int (*location_bearing_get_t)(const struct device *dev,
			struct location_bearing *bearing);

/**
 * @typedef location_speed_get_t
 * @brief API for getting newest speed data
 *
 * See struct location_speed for description of speed data
 */
typedef int (*location_speed_get_t)(const struct device *dev,
			struct location_speed *speed);

/**
 * @typedef location_altitude_get_t
 * @brief API for getting newest altitude data
 *
 * See struct location_altitude for description of altitude data
 */
typedef int (*location_altitude_get_t)(const struct device *dev,
			struct location_altitude *altitude);

/**
 * @brief Location events
 * 
 * Note that multiple events can be set simultaneously
 */
#define LOCATION_EVENT_POSITION_UPDATED BIT(0)
#define LOCATION_EVENT_BEARING_UPDATED BIT(1)
#define LOCATION_EVENT_SPEED_UPDATED BIT(2)
#define LOCATION_EVENT_ALTITUDE_UPDATED BIT(3)

/**
 * @typedef location_event_handler_t
 * @brief Callback API invoked when a location event is raised
 *
 * See struct location_provider_event for description of the event
 */
typedef void (*location_event_handler_t)(const struct device *dev,
			uint8_t events, void *user_data);

/**
 * @typedef location_event_handler_set_t
 * @brief API for setting the location event handler
 */
typedef int (*location_event_handler_set_t)(const struct device *dev,
			location_event_handler_t handler, void *user_data);

/**
 * @brief Location API
 */
__subsystem struct location_api {
	location_position_get_t position_get;
	location_bearing_get_t bearing_get;
	location_speed_get_t speed_get;
	location_altitude_get_t altitude_get;
	location_event_handler_set_t event_handler_set;
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
__syscall int location_position_get(const struct device *dev, struct location_position *position);

static inline int z_impl_location_position_get(const struct device *dev,
			struct location_position *position)
{
	if (dev == NULL || position == NULL) {
		return -EINVAL;
	}

	if (((const struct location_api *)dev->api)->position_get == NULL) {
		return -ENOSYS;
	}

	return ((const struct location_api *)dev->api)->position_get(dev, position);
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
__syscall int location_bearing_get(const struct device *dev, struct location_bearing *bearing);

static inline int z_impl_location_bearing_get(const struct device *dev,
			struct location_bearing *bearing)
{
	if (dev == NULL || bearing == NULL) {
		return -EINVAL;
	}

	if (((const struct location_api *)dev->api)->bearing_get == NULL) {
		return -ENOSYS;
	}

	return ((const struct location_api *)dev->api)->bearing_get(dev, bearing);
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
__syscall int location_speed_get(const struct device *dev, struct location_speed *speed);

static inline int z_impl_location_speed_get(const struct device *dev,
			struct location_speed *speed)
{
	if (dev == NULL || speed == NULL) {
		return -EINVAL;
	}

	if (((const struct location_api *)dev->api)->speed_get == NULL) {
		return -ENOSYS;
	}

	return ((const struct location_api *)dev->api)->speed_get(dev, speed);
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
__syscall int location_altitude_get(const struct device *dev, struct location_altitude *altitude);

static inline int z_impl_location_altitude_get(const struct device *dev,
			struct location_altitude *altitude)
{
	if (dev == NULL || altitude == NULL) {
		return -EINVAL;
	}

	if (((const struct location_api *)dev->api)->altitude_get == NULL) {
		return -ENOSYS;
	}

	return ((const struct location_api *)dev->api)->altitude_get(dev, altitude);
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
__syscall int location_event_handler_set(const struct device *dev,
			location_event_handler_t handler, void *user_data);

static inline int z_impl_location_event_handler_set(const struct device *dev,
			location_event_handler_t handler, void *user_data)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	if (((const struct location_api *)dev->api)->event_handler_set == NULL) {
		return -ENOSYS;
	}

	return ((const struct location_api *)dev->api)->event_handler_set(dev, handler, user_data);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/location.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_LOCATION_H_ */
