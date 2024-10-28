/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file gnss.h
 * @brief Public GNSS API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GNSS_H_
#define ZEPHYR_INCLUDE_DRIVERS_GNSS_H_

/**
 * @brief GNSS Interface
 * @defgroup gnss_interface GNSS Interface
 * @since 3.6
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/data/navigation.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/** GNSS PPS modes */
enum gnss_pps_mode {
	/** PPS output disabled */
	GNSS_PPS_MODE_DISABLED = 0,
	/** PPS output always enabled */
	GNSS_PPS_MODE_ENABLED = 1,
	/** PPS output enabled from first lock */
	GNSS_PPS_MODE_ENABLED_AFTER_LOCK = 2,
	/** PPS output enabled while locked */
	GNSS_PPS_MODE_ENABLED_WHILE_LOCKED = 3
};

/** API for setting fix rate */
typedef int (*gnss_set_fix_rate_t)(const struct device *dev, uint32_t fix_interval_ms);

/** API for getting fix rate */
typedef int (*gnss_get_fix_rate_t)(const struct device *dev, uint32_t *fix_interval_ms);

/** GNSS navigation modes */
enum gnss_navigation_mode {
	/** Dynamics have no impact on tracking */
	GNSS_NAVIGATION_MODE_ZERO_DYNAMICS = 0,
	/** Low dynamics have higher impact on tracking */
	GNSS_NAVIGATION_MODE_LOW_DYNAMICS = 1,
	/** Low and high dynamics have equal impact on tracking */
	GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS = 2,
	/** High dynamics have higher impact on tracking */
	GNSS_NAVIGATION_MODE_HIGH_DYNAMICS = 3
};

/** API for setting navigation mode */
typedef int (*gnss_set_navigation_mode_t)(const struct device *dev,
					  enum gnss_navigation_mode mode);

/** API for getting navigation mode */
typedef int (*gnss_get_navigation_mode_t)(const struct device *dev,
					  enum gnss_navigation_mode *mode);

/** Systems contained in gnss_systems_t */
enum gnss_system {
	/** Global Positioning System (GPS) */
	GNSS_SYSTEM_GPS = BIT(0),
	/** GLObal NAvigation Satellite System (GLONASS) */
	GNSS_SYSTEM_GLONASS = BIT(1),
	/** Galileo */
	GNSS_SYSTEM_GALILEO = BIT(2),
	/** BeiDou Navigation Satellite System */
	GNSS_SYSTEM_BEIDOU = BIT(3),
	/** Quasi-Zenith Satellite System (QZSS) */
	GNSS_SYSTEM_QZSS = BIT(4),
	/** Indian Regional Navigation Satellite System (IRNSS) */
	GNSS_SYSTEM_IRNSS = BIT(5),
	/** Satellite-Based Augmentation System (SBAS) */
	GNSS_SYSTEM_SBAS = BIT(6),
	/** Indoor Messaging System (IMES) */
	GNSS_SYSTEM_IMES = BIT(7),
};

/** Type storing bitmask of GNSS systems */
typedef uint32_t gnss_systems_t;

/** API for enabling systems */
typedef int (*gnss_set_enabled_systems_t)(const struct device *dev, gnss_systems_t systems);

/** API for getting enabled systems */
typedef int (*gnss_get_enabled_systems_t)(const struct device *dev, gnss_systems_t *systems);

/** API for getting enabled systems */
typedef int (*gnss_get_supported_systems_t)(const struct device *dev, gnss_systems_t *systems);

/** API for getting timestamp of last PPS pulse */
typedef int (*gnss_get_latest_timepulse_t)(const struct device *dev, k_ticks_t *timestamp);

/** GNSS fix status */
enum gnss_fix_status {
	/** No GNSS fix acquired */
	GNSS_FIX_STATUS_NO_FIX = 0,
	/** GNSS fix acquired */
	GNSS_FIX_STATUS_GNSS_FIX = 1,
	/** Differential GNSS fix acquired */
	GNSS_FIX_STATUS_DGNSS_FIX = 2,
	/** Estimated fix acquired */
	GNSS_FIX_STATUS_ESTIMATED_FIX = 3,
};

/** GNSS fix quality */
enum gnss_fix_quality {
	/** Invalid fix */
	GNSS_FIX_QUALITY_INVALID = 0,
	/** Standard positioning service */
	GNSS_FIX_QUALITY_GNSS_SPS = 1,
	/** Differential GNSS */
	GNSS_FIX_QUALITY_DGNSS = 2,
	/** Precise positioning service */
	GNSS_FIX_QUALITY_GNSS_PPS = 3,
	/** Real-time kinematic */
	GNSS_FIX_QUALITY_RTK = 4,
	/** Floating real-time kinematic */
	GNSS_FIX_QUALITY_FLOAT_RTK = 5,
	/** Estimated fix */
	GNSS_FIX_QUALITY_ESTIMATED = 6,
};

/** GNSS info data structure */
struct gnss_info {
	/** Number of satellites being tracked */
	uint16_t satellites_cnt;
	/** Horizontal dilution of precision in 1/1000 */
	uint32_t hdop;
	/** The fix status */
	enum gnss_fix_status fix_status;
	/** The fix quality */
	enum gnss_fix_quality fix_quality;
};

/** GNSS time data structure */
struct gnss_time {
	/** Hour [0, 23] */
	uint8_t hour;
	/** Minute [0, 59] */
	uint8_t minute;
	/** Millisecond [0, 60999] */
	uint16_t millisecond;
	/** Day of month [1, 31] */
	uint8_t month_day;
	/** Month [1, 12] */
	uint8_t month;
	/** Year [0, 99] */
	uint8_t century_year;
};

/** GNSS API structure */
__subsystem struct gnss_driver_api {
	gnss_set_fix_rate_t set_fix_rate;
	gnss_get_fix_rate_t get_fix_rate;
	gnss_set_navigation_mode_t set_navigation_mode;
	gnss_get_navigation_mode_t get_navigation_mode;
	gnss_set_enabled_systems_t set_enabled_systems;
	gnss_get_enabled_systems_t get_enabled_systems;
	gnss_get_supported_systems_t get_supported_systems;
	gnss_get_latest_timepulse_t get_latest_timepulse;
};

/** GNSS data structure */
struct gnss_data {
	/** Navigation data acquired */
	struct navigation_data nav_data;
	/** GNSS info when navigation data was acquired */
	struct gnss_info info;
	/** UTC time when data was acquired */
	struct gnss_time utc;
};

/** Template for GNSS data callback */
typedef void (*gnss_data_callback_t)(const struct device *dev, const struct gnss_data *data);

/** GNSS callback structure */
struct gnss_data_callback {
	/** Filter callback to GNSS data from this device if not NULL */
	const struct device *dev;
	/** Callback called when GNSS data is published */
	gnss_data_callback_t callback;
};

/** GNSS satellite structure */
struct gnss_satellite {
	/** Pseudo-random noise sequence */
	uint8_t prn;
	/** Signal-to-noise ratio in dB */
	uint8_t snr;
	/** Elevation in degrees [0, 90] */
	uint8_t elevation;
	/** Azimuth relative to True North in degrees [0, 359] */
	uint16_t azimuth;
	/** System of satellite */
	enum gnss_system system;
	/** True if satellite is being tracked */
	uint8_t is_tracked : 1;
};

/** Template for GNSS satellites callback */
typedef void (*gnss_satellites_callback_t)(const struct device *dev,
					   const struct gnss_satellite *satellites,
					   uint16_t size);

/** GNSS callback structure */
struct gnss_satellites_callback {
	/** Filter callback to GNSS data from this device if not NULL */
	const struct device *dev;
	/** Callback called when GNSS satellites is published */
	gnss_satellites_callback_t callback;
};

/**
 * @brief Set the GNSS fix rate
 *
 * @param dev Device instance
 * @param fix_interval_ms Fix interval to set in milliseconds
 *
 * @return 0 if successful
 * @return -errno negative errno code on failure
 */
__syscall int gnss_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms);

static inline int z_impl_gnss_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->set_fix_rate == NULL) {
		return -ENOSYS;
	}

	return api->set_fix_rate(dev, fix_interval_ms);
}

/**
 * @brief Get the GNSS fix rate
 *
 * @param dev Device instance
 * @param fix_interval_ms Destination for fix interval in milliseconds
 *
 * @return 0 if successful
 * @return -errno negative errno code on failure
 */
__syscall int gnss_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms);

static inline int z_impl_gnss_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->get_fix_rate == NULL) {
		return -ENOSYS;
	}

	return api->get_fix_rate(dev, fix_interval_ms);
}

/**
 * @brief Set the GNSS navigation mode
 *
 * @param dev Device instance
 * @param mode Navigation mode to set
 *
 * @return 0 if successful
 * @return -errno negative errno code on failure
 */
__syscall int gnss_set_navigation_mode(const struct device *dev,
				       enum gnss_navigation_mode mode);

static inline int z_impl_gnss_set_navigation_mode(const struct device *dev,
						  enum gnss_navigation_mode mode)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->set_navigation_mode == NULL) {
		return -ENOSYS;
	}

	return api->set_navigation_mode(dev, mode);
}

/**
 * @brief Get the GNSS navigation mode
 *
 * @param dev Device instance
 * @param mode Destination for navigation mode
 *
 * @return 0 if successful
 * @return -errno negative errno code on failure
 */
__syscall int gnss_get_navigation_mode(const struct device *dev,
				       enum gnss_navigation_mode *mode);

static inline int z_impl_gnss_get_navigation_mode(const struct device *dev,
						  enum gnss_navigation_mode *mode)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->get_navigation_mode == NULL) {
		return -ENOSYS;
	}

	return api->get_navigation_mode(dev, mode);
}

/**
 * @brief Set enabled GNSS systems
 *
 * @param dev Device instance
 * @param systems Systems to enable
 *
 * @return 0 if successful
 * @return -errno negative errno code on failure
 */
__syscall int gnss_set_enabled_systems(const struct device *dev, gnss_systems_t systems);

static inline int z_impl_gnss_set_enabled_systems(const struct device *dev,
						  gnss_systems_t systems)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->set_enabled_systems == NULL) {
		return -ENOSYS;
	}

	return api->set_enabled_systems(dev, systems);
}

/**
 * @brief Get enabled GNSS systems
 *
 * @param dev Device instance
 * @param systems Destination for enabled systems
 *
 * @return 0 if successful
 * @return -errno negative errno code on failure
 */
__syscall int gnss_get_enabled_systems(const struct device *dev, gnss_systems_t *systems);

static inline int z_impl_gnss_get_enabled_systems(const struct device *dev,
						  gnss_systems_t *systems)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->get_enabled_systems == NULL) {
		return -ENOSYS;
	}

	return api->get_enabled_systems(dev, systems);
}

/**
 * @brief Get supported GNSS systems
 *
 * @param dev Device instance
 * @param systems Destination for supported systems
 *
 * @return 0 if successful
 * @return -errno negative errno code on failure
 */
__syscall int gnss_get_supported_systems(const struct device *dev, gnss_systems_t *systems);

static inline int z_impl_gnss_get_supported_systems(const struct device *dev,
						    gnss_systems_t *systems)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->get_supported_systems == NULL) {
		return -ENOSYS;
	}

	return api->get_supported_systems(dev, systems);
}

/**
 * @brief Get the timestamp of the latest PPS timepulse
 *
 * @note The timestamp is considered valid when the timepulse pin is actively toggling.
 *
 * @param dev Device instance
 * @param timestamp Kernel tick count at the time of the PPS pulse
 *
 * @retval 0 if successful
 * @retval -ENOSYS if driver does not support API
 * @retval -ENOTSUP if driver does not have PPS pin connected
 * @retval -EAGAIN if PPS pulse is not considered valid
 */
__syscall int gnss_get_latest_timepulse(const struct device *dev, k_ticks_t *timestamp);

static inline int z_impl_gnss_get_latest_timepulse(const struct device *dev,
						    k_ticks_t *timestamp)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->get_latest_timepulse == NULL) {
		return -ENOSYS;
	}

	return api->get_latest_timepulse(dev, timestamp);
}

/**
 * @brief Register a callback structure for GNSS data published
 *
 * @param _dev Device pointer
 * @param _callback The callback function
 */
#if CONFIG_GNSS
#define GNSS_DATA_CALLBACK_DEFINE(_dev, _callback)                                              \
	static const STRUCT_SECTION_ITERABLE(gnss_data_callback,                                \
					     _gnss_data_callback__##_callback) = {              \
		.dev = _dev,                                                                    \
		.callback = _callback,                                                          \
	}
#else
#define GNSS_DATA_CALLBACK_DEFINE(_dev, _callback)
#endif

/**
 * @brief Register a callback structure for GNSS satellites published
 *
 * @param _dev Device pointer
 * @param _callback The callback function
 */
#if CONFIG_GNSS_SATELLITES
#define GNSS_SATELLITES_CALLBACK_DEFINE(_dev, _callback)                                        \
	static const STRUCT_SECTION_ITERABLE(gnss_satellites_callback,                          \
					     _gnss_satellites_callback__##_callback) = {        \
		.dev = _dev,                                                                    \
		.callback = _callback,                                                          \
	}
#else
#define GNSS_SATELLITES_CALLBACK_DEFINE(_dev, _callback)
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/gnss.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_GNSS_H_ */
