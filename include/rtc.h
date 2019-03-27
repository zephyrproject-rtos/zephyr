/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RTC_H_
#define ZEPHYR_INCLUDE_RTC_H_
#include <zephyr/types.h>
#include <device.h>
#include <misc/util.h>
#include <counter.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Number of RTC ticks in a second */
#define RTC_ALARM_SECOND (32768 / CONFIG_RTC_PRESCALER)

/** Number of RTC ticks in a minute */
#define RTC_ALARM_MINUTE (RTC_ALARM_SECOND * 60)

/** Number of RTC ticks in an hour */
#define RTC_ALARM_HOUR (RTC_ALARM_MINUTE * 60)

/** Number of RTC ticks in a day */
#define RTC_ALARM_DAY (RTC_ALARM_HOUR * 24)

typedef void (*rtc_callback_t)(struct device *dev);

struct rtc_config {
	u32_t init_val;
	/*!< enable/disable alarm  */
	u8_t alarm_enable;
	/*!< initial configuration value for the 32bit RTC alarm value  */
	u32_t alarm_val;
	/*!< Pointer to function to call when alarm value
	 * matches current RTC value */
	rtc_callback_t cb_fn;
};

typedef void (*rtc_api_enable)(struct device *dev);
typedef void (*rtc_api_disable)(struct device *dev);
typedef int (*rtc_api_set_config)(struct device *dev,
				  struct rtc_config *config);
typedef int (*rtc_api_set_alarm)(struct device *dev,
				 const u32_t alarm_val);
typedef u32_t (*rtc_api_read)(struct device *dev);
typedef u32_t (*rtc_api_get_pending_int)(struct device *dev);

struct rtc_driver_api {
	rtc_api_enable enable;
	rtc_api_disable disable;
	rtc_api_read read;
	rtc_api_set_config set_config;
	rtc_api_set_alarm set_alarm;
	rtc_api_get_pending_int get_pending_int;
};

__deprecated __syscall u32_t rtc_read(struct device *dev);

static inline u32_t z_impl_rtc_read(struct device *dev)
{
	return counter_read(dev);
}

__deprecated __syscall void rtc_enable(struct device *dev);

static inline void z_impl_rtc_enable(struct device *dev)
{
	counter_start(dev);
}

__deprecated __syscall void rtc_disable(struct device *dev);

static inline void z_impl_rtc_disable(struct device *dev)
{
	counter_stop(dev);
}

static inline void rtc_counter_top_callback(struct device *dev,
					     void *user_data)
{
	rtc_callback_t cb_fn = (rtc_callback_t)user_data;

	if (cb_fn) {
		cb_fn(dev);
	}
}

__deprecated static inline int rtc_set_config(struct device *dev,
					      struct rtc_config *cfg)
{
	int err;

	if (cfg->init_val) {
		return -ENOTSUP;
	}

	err = counter_set_top_value(dev, cfg->alarm_val,
				    rtc_counter_top_callback, cfg->cb_fn);

	if (err == 0 && cfg->alarm_enable != 0U) {
		err = counter_start(dev);
	}

	return err;
}

__deprecated __syscall int rtc_set_alarm(struct device *dev,
					 const u32_t alarm_val);

static inline int z_impl_rtc_set_alarm(struct device *dev,
				      const u32_t alarm_val)
{
	return counter_set_top_value(dev, alarm_val, rtc_counter_top_callback,
				     counter_get_user_data(dev));
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
 * @retval 1 if the rtc interrupt is pending.
 * @retval 0 if no rtc interrupt is pending.
 */
__deprecated __syscall int rtc_get_pending_int(struct device *dev);

static inline int z_impl_rtc_get_pending_int(struct device *dev)
{
	return counter_get_pending_int(dev);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/rtc.h>

#endif
