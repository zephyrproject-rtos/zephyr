/**
 * @file location.h
 *
 * @brief Public location API.
 *
 */

/*
 * Copyright (c) 2022, Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LOCATION_H_
#define ZEPHYR_INCLUDE_LOCATION_H_

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
 */
struct location_altitude {
	/** Altitude above WGS84 reference ellipsoid in millimeters */
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
 * @brief Location API
 */
__subsystem struct location_provider_api {
	location_position_get_t position_get;
	location_bearing_get_t bearing_get;
	location_speed_get_t speed_get;
	location_altitude_get_t altitude_get;
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Location provider structure
 */
struct location_provider {
	/** The device instance implementing the location api */
	const struct device *dev;
	/** The device implementation of the location api */
	const struct location_provider_api *api;
};

/** @endcond */

/**
 * @brief Bitfield specifying which parameters were updated
 */
union location_event {
	struct {
		uint8_t position : 1;
		uint8_t bearing  : 1;
		uint8_t speed    : 1;
		uint8_t altitude : 1;
	};
	uint8_t value;
};

/**
 * @typedef location_event_handler_t
 * @brief Callback API invoked when a location event is raised
 *
 * See struct location_provider_event for description of the event
 */
typedef void (*location_event_handler_t)(const struct location_provider *provider,
					      union location_event event);

/**
 * @brief Get the latest position from the location provider
 *
 * @param provider Location provider acquired using location_providers_get()
 * @param position Destination for position data
 * @return -EINVAL if any argument is NULL
 * @return -ENOSYS if api call is not implemented by driver
 * @return -ENOENT if no data is available
 * @return 0 if successful, negative errno code otherwise
 */
__syscall int location_position_get(const struct location_provider *provider,
				    struct location_position *position);

static inline int z_impl_location_position_get(const struct location_provider *provider,
					       struct location_position *position)
{
	if (provider == NULL || position == NULL) {
		return -EINVAL;
	}

	if (provider->api->position_get == NULL) {
		return -ENOSYS;
	}

	return provider->api->position_get(provider->dev, position);
}

/**
 * @brief Get the latest bearing from the location provider
 *
 * @param provider Location provider acquired using location_providers_get()
 * @param bearing Destination for bearing data
 * @return -EINVAL if any argument is NULL
 * @return -ENOSYS if api call is not implemented by driver
 * @return -ENOENT if no data is available
 * @return 0 if successful, negative errno code otherwise
 */
__syscall int location_bearing_get(const struct location_provider *provider,
				   struct location_bearing *bearing);

static inline int z_impl_location_bearing_get(const struct location_provider *provider,
					      struct location_bearing *bearing)
{
	if (provider == NULL || bearing == NULL) {
		return -EINVAL;
	}

	if (provider->api->bearing_get == NULL) {
		return -ENOSYS;
	}

	return provider->api->bearing_get(provider->dev, bearing);
}

/**
 * @brief Get the latest speed from the location provider
 *
 * This function finds and returns the most accurate speed
 * from any registered location provider
 *
 * @param provider Location provider acquired using location_providers_get()
 * @param speed Destination for speed data
 * @return -EINVAL if any argument is NULL
 * @return -ENOSYS if api call is not implemented by driver
 * @return -ENOENT if no data is available
 * @return 0 if successful, negative errno code otherwise
 */
__syscall int location_speed_get(const struct location_provider *provider,
				 struct location_speed *speed);

static inline int z_impl_location_speed_get(const struct location_provider *provider,
					    struct location_speed *speed)
{
	if (provider == NULL || speed == NULL) {
		return -EINVAL;
	}

	if (provider->api->speed_get == NULL) {
		return -ENOSYS;
	}

	return provider->api->speed_get(provider->dev, speed);
}

/**
 * @brief Get the latest altitude from the location provider
 *
 * This function finds and returns the most accurate altitude
 * from any registered location provider
 *
 * @param provider Location provider acquired using location_providers_get()
 * @param altitude Destination for altitude data
 * @return -EINVAL if any argument is NULL
 * @return -ENOSYS if api call is not implemented by driver
 * @return -ENOENT if no data is available
 * @return 0 if successful, negative errno code otherwise
 */
__syscall int location_altitude_get(const struct location_provider *provider,
				    struct location_altitude *altitude);

static inline int z_impl_location_altitude_get(const struct location_provider *provider,
					       struct location_altitude *altitude)
{
	if (provider == NULL || altitude == NULL) {
		return -EINVAL;
	}

	if (provider->api->altitude_get == NULL) {
		return -ENOSYS;
	}

	return provider->api->altitude_get(provider->dev, altitude);
}

/**
 * @brief Get the device instance associated with provided locacation provider
 *
 * This function finds and returns the most accurate altitude
 * from any registered location provider
 *
 * @param provider The location provider
 * @return Pointer to the associated device instance
 */
__syscall const struct device *location_provider_device_get(
	const struct location_provider *provider);

static inline const struct device *z_impl_location_provider_device_get(
	const struct location_provider *provider)
{
	return provider->dev;
}

/**
 * @brief Register event handler to location event
 *
 * @param handler The handler which is invoked when a location event occurs
 * @param event_filter Specifies which parameters invoke the location event handler
 * @return -EINVAL if handler is NULL or no event are enabled
 * @return -ENOMEM if handler could not be registered
 * @return 0 if successful
 */
__syscall int location_event_handler_register(location_event_handler_t handler,
							union location_event event_filter);

int z_internal_location_event_handler_register(location_event_handler_t handler,
							union location_event event_filter);

static inline int z_impl_location_event_handler_register(
							location_event_handler_t handler,
							union location_event event_filter)
{
	return z_internal_location_event_handler_register(handler, event_filter);
}

/**
 * @brief Unregister event handler to location event
 *
 * @param handler The handler which will be unregistered
 * @return -EINVAL if handler is NULL
 * @return 0 if successful
 */
__syscall int location_event_handler_unregister(location_event_handler_t handler);

int z_internal_location_event_handler_unregister(location_event_handler_t handler);

static inline int z_impl_location_event_handler_unregister(
							location_event_handler_t handler)
{
	return z_internal_location_event_handler_unregister(handler);
}

/**
 * @brief Get an array of all registered location providers
 *
 * @param providers Destination for pointer to array of location providers
 * @return -EINVAL if any argument is NULL
 * @return Length of location providers array if successful
 */
__syscall int location_providers_get(const struct location_provider **providers);

int z_internal_location_providers_get(const struct location_provider **providers);

static inline int z_impl_location_providers_get(const struct location_provider **providers)
{
	return z_internal_location_providers_get(providers);
}

/**
 * @brief Register a location provider
 *
 * This function tries to register a location provider
 *
 * @param dev The device instance providing the location
 * @param api The location provider api implementation used by dev
 * @return -EINVAL if any argument is NULL
 * @return -ENOMEM if provider could not be registered
 * @return 0 if successful
 */
int location_provider_register(const struct device *dev,
			       const struct location_provider_api *api);

/**
 * @brief Raise location event
 *
 * This function raises a location event
 *
 * @param dev The previously registered device which is raising the event
 * @param event The event data
 * @return -EINVAL if handler is NULL or no event are set
 * @return -ENOENT if device is not associated with any registered location provider
 * @return 0 if successful
 */
int location_provider_raise_event(const struct device *dev, union location_event event);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/location.h>

#endif /* ZEPHYR_INCLUDE_LOCATION_H_ */
