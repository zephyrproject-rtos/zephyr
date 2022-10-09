/*
 * Copyright (c) 2022, tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for input devices.
 * The scope of this API is report which input event was triggered
 * and users can resolve input event by event type in their application.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INPUT_H_
#define ZEPHYR_INCLUDE_DRIVERS_INPUT_H_

#include <time.h>
#include <stddef.h>
#include <zephyr/types.h>
#include <zephyr/device.h>


#ifdef CONFIG_NEWLIB_LIBC

#include <newlib.h>

#ifdef __NEWLIB__
#include <sys/_timeval.h>
#else /* __NEWLIB__ */
#include <sys/types.h>
/* workaround for older Newlib 2.x, as it lacks sys/_timeval.h */
struct timeval {
	time_t tv_sec;
	suseconds_t tv_usec;
};
#endif /* __NEWLIB__ */

#else /* CONFIG_NEWLIB_LIBC */

#ifdef CONFIG_ARCH_POSIX
#include <bits/types/struct_timeval.h>
#else
#include <sys/_timeval.h>
#endif

#endif /* CONFIG_NEWLIB_LIBC */


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief INPUT APIs
 * @defgroup input_interface Input Subsystem Driver APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Input attribute types.
 *
 */
enum input_attr_type {
	/* Input driver hardware chipid, readonly. */
	INPUT_ATTR_HARDWARE_CHIPID,
	/* Input driver firmware version, readonly. */
	INPUT_ATTR_FIRMWARE_VERSION,
	/* Input device event read timeout */
	INPUT_ATTR_EVENT_READ_TIMEOUT,
};

/**
 * @brief Input attribute data union.
 *
 */
union input_attr_data {
	uint32_t chipid;
	uint32_t version;
	/* Waiting period to read event, or one of the special values K_NO_WAIT and K_FOREVER. */
	k_timeout_t timeout;
	/**
	 * This is required to convert between custom data and attribute data.
	 * Custom data size must be less than or equal to CONFIG_INPUT_ATTR_DATA_SIZE.
	 */
	uint32_t userdata[CONFIG_INPUT_ATTR_DATA_SIZE / sizeof(uint32_t)];
};

BUILD_ASSERT(sizeof(union input_attr_data) <= CONFIG_INPUT_ATTR_DATA_SIZE);

/**
 * @brief Representation of a input event.
 *
 */
struct input_event {
	/* The time of the event, the time is the system uptime */
	struct timeval time;
	/* event type (EV_SYN, EV_KEY, EV_ABS, etc) */
	uint16_t type;
	/* event code (Keyboard code, ABS code, etc) */
	uint16_t code;
	/* event value (Keyboard value, ABS position, etc) */
	int32_t value;
};

#define input_event_size	sizeof(struct input_event)

/**
 * @brief Event type
 *
 */
#define EV_SYN			0x00
#define EV_KEY			0x01
#define EV_REL			0x02
#define EV_ABS			0x03

/**
 * @brief Synchronization events
 *
 */
#define SYN_REPORT		0x00

/**
 * @brief Relative axes
 *
 */
#define REL_X			0x00
#define REL_Y			0x01
#define REL_Z			0x02

/**
 * @brief Absolute axes
 *
 */
#define ABS_X			0x00
#define ABS_Y			0x01
#define ABS_Z			0x02

/**
 * @brief Event Key value
 *
 */
enum keyboard_value {
	KEY_RELEASE,
	KEY_PRESSED,
	KEY_LONG_PRESSED,
	KEY_HOLD_PRESSED,
	KEY_LONG_RELEASE
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Input subsystem driver API definition and system call entry points.
 *
 * (Internal use only.)
 */
typedef int (*input_setup_t)(const struct device *dev);
typedef int (*input_release_t)(const struct device *dev);
typedef int (*input_attr_get_t)(const struct device *dev,
			enum input_attr_type type, union input_attr_data *data);
typedef int (*input_attr_set_t)(const struct device *dev,
			enum input_attr_type type, union input_attr_data *data);
typedef int (*input_event_read_t)(const struct device *dev, struct input_event *event);
typedef int (*input_event_write_t)(const struct device *dev, struct input_event *event);

__subsystem struct input_driver_api {
	input_setup_t setup;
	input_release_t release;
	input_attr_get_t attr_get;
	input_attr_set_t attr_set;
	input_event_read_t event_read;
	input_event_write_t event_write;
};
/**
 * @endcond
 */

/**
 * @brief Setup a input device instance.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int input_setup(const struct device *dev);

static inline int z_impl_input_setup(const struct device *dev)
{
	const struct input_driver_api *api =
				(struct input_driver_api *)dev->api;

	return api->setup(dev);
}

/**
 * @brief Release a input device instance.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int input_release(const struct device *dev);

static inline int z_impl_input_release(const struct device *dev)
{
	const struct input_driver_api *api =
				(struct input_driver_api *)dev->api;

	return api->release(dev);
}

/**
 * @brief Get attribute from the input device instance.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param type The type of attribute which get from the driver instance.
 * @param data Pointer to the attribute data structure which get from the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int input_attr_get(const struct device *dev,
			enum input_attr_type type, union input_attr_data *data);

static inline int z_impl_input_attr_get(const struct device *dev,
			enum input_attr_type type, union input_attr_data *data)
{
	const struct input_driver_api *api =
				(struct input_driver_api *)dev->api;

	return api->attr_get(dev, type, data);
}

/**
 * @brief Set attribute to the input device instance.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param type The type of attribute which set to the driver instance.
 * @param data Pointer to the attribute data structure which set to the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int input_attr_set(const struct device *dev,
			enum input_attr_type type, union input_attr_data *data);

static inline int z_impl_input_attr_set(const struct device *dev,
			enum input_attr_type type, union input_attr_data *data)
{
	const struct input_driver_api *api =
				(struct input_driver_api *)dev->api;

	return api->attr_set(dev, type, data);
}

/**
 * @brief Read event from the input device instance.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param event Pointer to the event structure which read from the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int input_event_read(const struct device *dev, struct input_event *event);

static inline int z_impl_input_event_read(const struct device *dev, struct input_event *event)
{
	const struct input_driver_api *api =
				(struct input_driver_api *)dev->api;

	return api->event_read(dev, event);
}

/**
 * @brief Write event to the input device instance. Used to simulate input events.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param event Pointer to the event structure which write to the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int input_event_write(const struct device *dev, struct input_event *event);

static inline int z_impl_input_event_write(const struct device *dev, struct input_event *event)
{
	const struct input_driver_api *api =
				(struct input_driver_api *)dev->api;

	return api->event_write(dev, event);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/input.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_INPUT_H_ */
