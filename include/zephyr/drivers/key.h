/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public KEY driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_KEY_H_
#define ZEPHYR_INCLUDE_DRIVERS_KEY_H_

/**
 * @brief KEY Interface
 * @defgroup key_interface Key Driver APIs
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	KEY_EVENT_NONE,
    KEY_EVENT_PRESSED,
    KEY_EVENT_RELEASE,
    KEY_EVENT_LONG_PRESSED,
    KEY_EVENT_LONG_RELEASE,
    KEY_EVENT_HOLD_PRESSED,
    KEY_EVENT_MAXNBR,
} key_event_t;

/**
 * @brief Key callback called when user press/release a key on a keybaord.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param code Describes key code.
 * @param event Describes the kind of key event.
 */
typedef void (*key_callback_t)(const struct device *dev, uint16_t code, key_event_t event);

/**
 * @cond INTERNAL_HIDDEN
 *
 * Keyboard scan driver API definition and system call entry points.
 *
 * (Internal use only.)
 */
typedef int (*key_setup_t)(const struct device *dev, key_callback_t callback);
typedef int (*key_remove_t)(const struct device *dev);

__subsystem struct key_driver_api {
	key_setup_t  setup;
	key_remove_t remove;
};
/**
 * @endcond
 */

/**
 * @brief Setup a key instance.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback called when keyboard devices reply to to a keyboard
 * event such as key pressed/released.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int key_setup(const struct device *dev, key_callback_t callback);

static inline int z_impl_key_setup(const struct device *dev, key_callback_t callback)
{
	const struct key_driver_api *api =
				(struct key_driver_api *)dev->api;

	if (api->setup == NULL) {
		return -ENOSYS;
	}

	return api->setup(dev, callback);
}

/**
 * @brief Remove a key instance.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int key_remove(const struct device *dev);

static inline int z_impl_key_remove(const struct device *dev)
{
	const struct key_driver_api *api =
				(struct key_driver_api *)dev->api;

	if (api->remove == NULL) {
		return -ENOSYS;
	}

	return api->remove(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/key.h>

#endif	/* ZEPHYR_INCLUDE_DRIVERS_KEY_H_ */
