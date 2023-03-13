/*
 * Copyright (C) 2022 Ryan Walker <info@interruptlabs.ca>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file drivers/touch.h
 *
 * @brief Public APIs for the touch driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_TOUCH_H_
#define ZEPHYR_INCLUDE_DRIVERS_TOUCH_H_

/**
 * @brief Touch Interface
 * @defgroup touch_interface Touch Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*single_tap_get_t)(const struct device *dev);
typedef bool (*two_finger_tap_get_t)(const struct device *dev);
typedef int16_t (*x_pos_get_abs_t)(const struct device *dev);
typedef int16_t (*y_pos_get_abs_t)(const struct device *dev);
typedef int16_t (*x_pos_get_rel_t)(const struct device *dev);
typedef int16_t (*y_pos_get_rel_t)(const struct device *dev);
typedef int (*num_fingers_get_t)(const struct device *dev);

__subsystem struct touch_driver_api {
	single_tap_get_t single_tap;
	two_finger_tap_get_t two_finger_tap;
	x_pos_get_abs_t x_pos_abs;
	y_pos_get_abs_t y_pos_abs;
	x_pos_get_rel_t x_pos_rel;
	y_pos_get_rel_t y_pos_rel;
	num_fingers_get_t num_fingers;
};

__syscall bool single_tap_get(const struct device *dev);
__syscall bool two_finger_tap_get(const struct device *dev);
__syscall int16_t x_pos_get_abs(const struct device *dev);
__syscall int16_t y_pos_get_abs(const struct device *dev);
__syscall int16_t x_pos_get_rel(const struct device *dev);
__syscall int16_t y_pos_get_rel(const struct device *dev);
__syscall int num_fingers_get(const struct device *dev);

static inline bool z_impl_single_tap_get(const struct device *dev)
{
	const struct touch_driver_api *api = (const struct touch_driver_api *)dev->api;

	return api->single_tap(dev);
}

static inline bool z_impl_two_finger_tap_get(const struct device *dev)
{
	const struct touch_driver_api *api = (const struct touch_driver_api *)dev->api;

	return api->two_finger_tap(dev);
}

static inline int z_impl_num_fingers_get(const struct device *dev)
{
	const struct touch_driver_api *api = (const struct touch_driver_api *)dev->api;

	return api->num_fingers(dev);
}

static inline int16_t z_impl_x_pos_get_abs(const struct device *dev)
{
	const struct touch_driver_api *api = (const struct touch_driver_api *)dev->api;

	return api->x_pos_abs(dev);
}

static inline int16_t z_impl_y_pos_get_abs(const struct device *dev)
{
	const struct touch_driver_api *api = (const struct touch_driver_api *)dev->api;

	return api->y_pos_abs(dev);
}

static inline int16_t z_impl_x_pos_get_rel(const struct device *dev)
{
	const struct touch_driver_api *api = (const struct touch_driver_api *)dev->api;

	return api->x_pos_rel(dev);
}

static inline int16_t z_impl_y_pos_get_rel(const struct device *dev)
{
	const struct touch_driver_api *api = (const struct touch_driver_api *)dev->api;

	return api->y_pos_rel(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/touch.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_TOUCH_H_ */
