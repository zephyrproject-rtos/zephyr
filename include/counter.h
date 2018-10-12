/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for counter and timer drivers
 */

#ifndef ZEPHYR_INCLUDE_COUNTER_H_
#define ZEPHYR_INCLUDE_COUNTER_H_

/**
 * @brief Counter Interface
 * @defgroup counter_interface Counter Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*counter_callback_t)(struct device *dev, void *user_data);

typedef int (*counter_api_start)(struct device *dev);
typedef int (*counter_api_stop)(struct device *dev);
typedef u32_t (*counter_api_read)(struct device *dev);
typedef int (*counter_api_set_alarm)(struct device *dev,
				     counter_callback_t callback,
				     u32_t count, void *user_data);
typedef u32_t (*counter_api_get_pending_int)(struct device *dev);

struct counter_driver_api {
	counter_api_start start;
	counter_api_stop stop;
	counter_api_read read;
	counter_api_set_alarm set_alarm;
	counter_api_get_pending_int get_pending_int;
};

/**
 * @brief Start counter device in free running mode.
 *
 * Start the counter device. If the device is a 'countup' counter, the
 * counter initial value is set to zero. If it is a 'countdown' counter,
 * the initial value is set to the maximum value supported by the device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int counter_start(struct device *dev);

static inline int _impl_counter_start(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->start(dev);
}

/**
 * @brief Stop counter device.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval -ENODEV if the device doesn't support stopping the
 *                        counter.
 */
__syscall int counter_stop(struct device *dev);

static inline int _impl_counter_stop(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->stop(dev);
}

/**
 * @brief Read current counter value.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return  32-bit value
 */
__syscall u32_t counter_read(struct device *dev);

static inline u32_t _impl_counter_read(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->read(dev);
}

/**
 * @brief Set an alarm.
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback Pointer to the callback function. If this is NULL,
 *                 this function unsets the alarm.
 * @param count Number of counter ticks. This is relative value, meaning
 *              an alarm will be triggered count ticks in the future.
 * @param user_data Pointer to user data.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if the counter was not started yet.
 * @retval -ENODEV if the device doesn't support interrupt (e.g. free
 *                        running counters).
 * @retval Negative errno code if failure.
 */
static inline int counter_set_alarm(struct device *dev,
				    counter_callback_t callback,
				    u32_t count, void *user_data)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->set_alarm(dev, callback, count, user_data);
}

/**
 * @brief Function to get pending interrupts
 *
 * The purpose of this function is to return the interrupt
 * status register for the device.
 * This is especially useful when waking up from
 * low power states to check the wake up source.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 1 if the counter interrupt is pending.
 * @retval 0 if no counter interrupt is pending.
 */
__syscall int counter_get_pending_int(struct device *dev);

static inline int _impl_counter_get_pending_int(struct device *dev)
{
	struct counter_driver_api *api;

	api = (struct counter_driver_api *)dev->driver_api;
	return api->get_pending_int(dev);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/counter.h>

#endif /* ZEPHYR_INCLUDE_COUNTER_H_ */
