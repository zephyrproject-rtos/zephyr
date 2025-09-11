/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the Renesas LVD voltage monitor
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_RENESAS_LVD_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_RENESAS_LVD_H_

/**
 * @brief Renesas LVD driver public APIs
 * @defgroup renesas_lvd_interface Renesas LVD driver APIs
 * @ingroup misc_interfaces
 * @{
 */

#include <stdint.h>
#include <zephyr/sys/slist.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief LVD voltage position relative to threshold.
 *
 *  This enum indicates whether the monitored voltage is
 *  currently above or below the configured threshold.
 */
typedef enum {
	/** Voltage is above threshold. */
	RENESAS_LVD_POSITION_ABOVE = 0,

	/** Voltage is below threshold. */
	RENESAS_LVD_POSITION_BELOW,
} renesas_lvd_position_t;

/** @brief LVD Voltage crossing status.
 *
 *  This enum defines the status, whether the voltage crossed
 *  the voltage detection level or not
 */
typedef enum {
	/** No threshold crossing detected */
	RENESAS_LVD_CROSS_NONE = 0,

	/** Voltage crossed over the threshold. */
	RENESAS_LVD_CROSS_OVER,
} renesas_lvd_cross_t;

/** @brief Renesas LVD generic status structure.
 *
 *  This structure combines the current voltage position and
 *  the last detected crossing type for the monitored channel.
 */
typedef struct {
	renesas_lvd_position_t position;
	renesas_lvd_cross_t cross;
} renesas_lvd_status_t;

typedef void (*renesas_lvd_callback_t)(const struct device *dev, void *user_data);

/**
 * @cond INTERNAL_HIDDEN
 *
 * Renesas LVD driver API definition and system call entry points
 *
 * (Internal use only.)
 */
__subsystem struct renesas_lvd_driver_api {
	int (*get_status)(const struct device *dev, renesas_lvd_status_t *status);
	int (*clear_status)(const struct device *dev);
	int (*callback_set)(const struct device *dev, renesas_lvd_callback_t callback,
			    void *user_data);
};

/**
 * @endcond
 */

/**
 * @brief Get the current state of the monitor.
 *
 * This function requests the Renesas LVD to get the current status.
 *
 * @param dev The Renesas LVD device.
 * @param status Pointer to the status structure to be filled.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int renesas_lvd_get_status(const struct device *dev, renesas_lvd_status_t *status);

static inline int z_impl_renesas_lvd_get_status(const struct device *dev,
						renesas_lvd_status_t *status)
{
	return DEVICE_API_GET(renesas_lvd, dev)->get_status(dev, status);
}

/**
 * @brief Clear the current status of the monitor.
 *
 * This function requests the Renesas LVD to clear the current status.
 *
 * @param dev The Renesas LVD device.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int renesas_lvd_clear_status(const struct device *dev);

static inline int z_impl_renesas_lvd_clear_status(const struct device *dev)
{
	return DEVICE_API_GET(renesas_lvd, dev)->clear_status(dev);
}

/**
 * @brief Set callback function for voltage detection.
 *
 * This function set callback function for voltage detection.
 *
 * @param dev The Renesas LVD device.
 * @param callback Pointer to the callback function to be set.
 * @param user_data Pointer to user data to be passed to the callback function.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int renesas_lvd_callback_set(const struct device *dev, renesas_lvd_callback_t callback,
				       void *user_data);

static inline int z_impl_renesas_lvd_callback_set(const struct device *dev,
						  renesas_lvd_callback_t callback, void *user_data)
{
	return DEVICE_API_GET(renesas_lvd, dev)->callback_set(dev, callback, user_data);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/renesas_lvd.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_RENESAS_LVD_H_ */
