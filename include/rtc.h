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



struct rtc_config {
	u32_t init_val;
	/*!< enable/disable alarm  */
	u8_t alarm_enable;
	/*!< initial configuration value for the 32bit RTC alarm value  */
	u32_t alarm_val;
	/*!< Pointer to function to call when alarm value
	 * matches current RTC value */
	void (*cb_fn)(struct device *dev);
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

__syscall u32_t rtc_read(struct device *dev);

static inline u32_t _impl_rtc_read(struct device *dev)
{
	const struct rtc_driver_api *api = dev->driver_api;

	return api->read(dev);
}

__syscall void rtc_enable(struct device *dev);

static inline void _impl_rtc_enable(struct device *dev)
{
	const struct rtc_driver_api *api = dev->driver_api;

	api->enable(dev);
}

__syscall void rtc_disable(struct device *dev);

static inline void _impl_rtc_disable(struct device *dev)
{
	const struct rtc_driver_api *api = dev->driver_api;

	api->disable(dev);
}

static inline int rtc_set_config(struct device *dev,
				 struct rtc_config *cfg)
{
	const struct rtc_driver_api *api = dev->driver_api;

	return api->set_config(dev, cfg);
}

__syscall int rtc_set_alarm(struct device *dev, const u32_t alarm_val);

static inline int _impl_rtc_set_alarm(struct device *dev,
				      const u32_t alarm_val)
{
	const struct rtc_driver_api *api = dev->driver_api;

	return api->set_alarm(dev, alarm_val);
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
__syscall int rtc_get_pending_int(struct device *dev);

static inline int _impl_rtc_get_pending_int(struct device *dev)
{
	struct rtc_driver_api *api;

	api = (struct rtc_driver_api *)dev->driver_api;
	return api->get_pending_int(dev);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/rtc.h>

#endif
