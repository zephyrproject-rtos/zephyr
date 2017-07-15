/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for counter and timer drivers
 */

#ifndef __COUNTER_H__
#define __COUNTER_H__

/**
 * @brief COUNTER Interface
 * @defgroup counter_interface COUNTER Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct counter_config {
	/* Counter frequency divider */
	u32_t divider;
	u32_t init_val;
};

typedef void (*counter_callback_t)(struct device *dev, void *user_data);

typedef int (*counter_api_config)(struct device *dev,
				  struct counter_config *config);
typedef int (*counter_api_start)(struct device *dev);
typedef int (*counter_api_stop)(struct device *dev);
typedef u32_t (*counter_api_read)(struct device *dev);
typedef int (*counter_api_set_alarm)(struct device *dev,
				     counter_callback_t callback,
				     u32_t count, void *user_data);
typedef int (*counter_api_cancel_alarm)(struct device *dev,
					int alarm_descriptor);
typedef u32_t (*counter_api_get_pending_int)(struct device *dev);

struct counter_driver_api {
	counter_api_config config;
	counter_api_start start;
	counter_api_stop stop;
	counter_api_read read;
	counter_api_set_alarm set_alarm;
	counter_api_cancel_alarm cancel_alarm;
	counter_api_get_pending_int get_pending_int;
};

/**
 * @brief Configure counter.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param config Pointer to the configuration structure.
 *
 * @retval 0 If successful.
 * @retval -EBUSY If there is still any alarm set.
 * @retval Negative errno code if failure.
 */
static inline int counter_config(struct device *dev,
				 struct counter_config *config)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->config(dev, config);
}

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
static inline int counter_start(struct device *dev)
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
static inline int counter_stop(struct device *dev)
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
static inline u32_t counter_read(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->read(dev);
}

/**
 * @brief Set an alarm.
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback Pointer to the callback function. if this is NULL,
 *                 this function unsets the alarm.
 * @param count Number of counter ticks.
 * @param user_data Pointer to user data.
 *
 * @retval 0 or positive - alarm descriptor (used to cancel alarm).
 * @retval -ENOTSUP if the counter was not started yet.
 * @retval -ENODEV if the device doesn't support interrupt (e.g. free
 *                        running counters).
 * @retval -ECANCELED if alarm can not be set due to some specific reasons
 * @retval Negative errno code if failure.
 *
 * @note Nordic Semiconductor SoCs: -ECANCELED is returned if alarm value is
 *	 closer to the current RTC counter value than RTC_MIN_DELTA, or alarm
 *	 value is greater then the currect RTC counter value + UINT32_MAX/2.
 */
static inline int counter_set_alarm(struct device *dev,
				    counter_callback_t callback,
				    u32_t count, void *user_data)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->set_alarm(dev, callback, count, user_data);
}

/**
 * @brief Cancel an alarm.
 * @param dev Pointer to the device structure for the driver instance.
 * @param alarm_descriptor number indicating which alarm should be cancelled.
 *
 * @retval 0 if success.
 * @retval -EINVAL if supplied alarm descriptor is not within supported range.
 * @retval -ENOTSUP if the alarm with passed alarm descriptor is not active.
 */
static inline int counter_cancel_alarm(struct device *dev,
			       int alarm_descriptor)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->cancel_alarm(dev, alarm_descriptor);
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
static inline int counter_get_pending_int(struct device *dev)
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

#endif /* __COUNTER_H__ */
