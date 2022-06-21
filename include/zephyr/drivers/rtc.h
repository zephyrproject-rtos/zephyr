/*
 * Copyright (c) 2022 Bjarki Arge Andreasen <bjarkix123@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file drivers/rtc.h
 * @brief Public Real time clock driver API
 */

#ifndef ZEPHYR_INCLUDE_RTC_H_
#define ZEPHYR_INCLUDE_RTC_H_

/**
 * @brief RTC Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/data/datetime.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Alarm triggered handler
 *
 * This handler is likely to be invoked from within an ISR context
 *
 * @param dev The device invoking the handler
 * @param user_data The optional user_data provided in the alarm setup
 */
typedef void (*rtc_alarm_triggered_handler_t)(const struct device *dev,
			void *user_data);

/**
 * @brief RTC alarm setup
 */
struct rtc_alarm_setup {
	/** Datetime to match */
	struct datetime *time;
	/** Parameters of the datetime to match */
	union datetime_mask mask;
	/** Handler invoked once the alarm is triggered */
	rtc_alarm_triggered_handler_t handler;
	/** Optional user data which passed to the handler when invoked */
	void *user_data;
};

/**
 * @brief RTC wakeup timer setup
 */
struct rtc_wakeup_timer_setup {
	/** Timeout in milliseconds */
	uint32_t timeout;
	/** Configure the wakeup timer to trigger periodically */
	bool periodic;
};

/**
 * @typedef rtc_time_set_t
 * @brief API for setting RTC date and time
 *
 * @param dev The RTC device to configure
 * @param dt The date and time to set
 */
typedef int (*rtc_time_set_t)(const struct device *dev,
			const struct datetime *dt);

/**
 * @typedef rtc_time_get_t
 * @brief API for getting RTC date and time
 *
 * @param dev The RTC device to get the time from
 * @param dt Destination for the current date and time
 */
typedef int (*rtc_time_get_t)(const struct device *dev,
		struct datetime *dt);

/**
 * @typedef rtc_alarm_set_t
 * @brief API for setting an RTC alarm
 *
 * @param dev The RTC device to configure
 * @param alarm The alarm setup to set
 */
typedef int (*rtc_alarm_set_t)(const struct device *dev,
		const struct rtc_alarm_setup *alarm);

/**
 * @typedef rtc_alarm_cancel_t
 * @brief API for cancelling RTC alarm
 *
 * @param dev The RTC device to configure
 * @param alarm_id The alarm id recevied when the alarm was set
 */
typedef int (*rtc_alarm_cancel_t)(const struct device *dev, int alarm_id);

/**
 * @typedef rtc_wakeup_timer_set_t
 * @brief API for setting the RTC wakeup timer
 *
 * @param dev The RTC device to configure
 * @param dtr The timer setup to set
 */
typedef int (*rtc_wakeup_timer_set_t)(const struct device *dev,
		const struct rtc_wakeup_timer_setup *timer);

/**
 * @typedef rtc_wakeup_timer_cancel_t
 * @brief API for cancelling the RTC wakeup timer
 *
 * @param dev The RTC device to configure
 */
typedef int (*rtc_wakeup_timer_cancel_t)(const struct device *dev);

__subsystem struct rtc_driver_api {
	rtc_time_set_t time_set;
	rtc_time_get_t time_get;
	rtc_alarm_set_t alarm_set;
	rtc_alarm_cancel_t alarm_cancel;
	rtc_wakeup_timer_set_t wakeup_timer_set;
	rtc_wakeup_timer_cancel_t wakeup_timer_cancel;
};

/**
 * @brief Set RTC date and time
 *
 * @param dev The RTC device to configure
 * @param dt The date and time to set
 *
 * @return 0 if successful, negative error code otherwise
 */
__syscall int rtc_time_set(const struct device *dev, const struct datetime *dt);

static inline int z_impl_rtc_time_set(const struct device *dev, const struct datetime *dt)
{
	if (dev == NULL || datetime == NULL) {
		return -EINVAL;
	}

	struct rtc_driver_api *api = (struct rtc_driver_api *)dev->api;

	if (api == NULL) {
		return -ENOSYS;
	}

	return api->time_set(dev, datetime);
}

/**
 * @brief Get RTC date and time
 *
 * @param dev The RTC device to get the time from
 * @param dt Destination for the current date and time
 *
 * @return 0 if successful, negative error code otherwise
 */
__syscall int rtc_time_get(const struct device *dev, struct datetime *dt);

static inline int z_impl_rtc_time_get(const struct device *dev, struct datetime *dt)
{
	if (dev == NULL || datetime == NULL) {
		return -EINVAL;
	}

	struct rtc_driver_api *api = (struct rtc_driver_api *)dev->api;

	if (api == NULL) {
		return -ENOSYS;
	}

	return api->time_get(dev, datetime);
}

/**
 * @brief Set RTC alarm
 *
 * @param dev The RTC device to configure
 * @param alarm The alarm setup to set
 *
 * @return Alarm id if successful, negative error code otherwise
 */
__syscall int rtc_alarm_set(const struct device *dev, const struct rtc_alarm_setup *alarm);

static inline int z_impl_rtc_alarm_set(const struct device *dev,
			const struct rtc_alarm_setup *alarm)
{
	if (dev == NULL || alarm == NULL) {
		return -EINVAL;
	}

	struct rtc_driver_api *api = (struct rtc_driver_api *)dev->api;

	if (api == NULL) {
		return -ENOSYS;
	}

	return api->alarm_set(dev, alarm);
}

/**
 * @brief Get RTC alarm
 *
 * @param dev The RTC device to get the time from
 * @param alarm_id The alarm id recevied when the alarm was set
 *
 * @return 0 if successful, negative error code otherwise
 */
__syscall int rtc_alarm_cancel(const struct device *dev, int alarm_id);

static inline int z_impl_rtc_alarm_cancel(const struct device *dev, int alarm_id)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	struct rtc_driver_api *api = (struct rtc_driver_api *)dev->api;

	if (api == NULL) {
		return -ENOSYS;
	}

	return api->alarm_cancel(dev, alarm_id);
}

/**
 * @brief Set RTC wakeup timer
 *
 * @param dev The RTC device to configure
 * @param dtr The timer setup to set
 *
 * @return 0 if successful, negative error code otherwise
 */
__syscall int rtc_wakeup_timer_set(const struct device *dev, const struct rtc_wakeup_timer_setup *timer);

static inline int z_impl_rtc_wakeup_timer_set(const struct device *dev,
			const struct rtc_wakeup_timer_setup *timer)
{
	if (dev == NULL || timer == NULL) {
		return -EINVAL;
	}

	struct rtc_driver_api *api = (struct rtc_driver_api *)dev->api;

	if (api == NULL) {
		return -ENOSYS;
	}

	return api->wakeup_timer_set(dev, timer);
}

/**
 * @brief Cancel RTC wakeup timer
 *
 * @param dev The RTC device to configure
 *
 * @return 0 if successful, negative error code otherwise
 */
__syscall int rtc_wakeup_timer_cancel(const struct device *dev);

static inline int z_impl_rtc_wakeup_timer_cancel(const struct device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	struct rtc_driver_api *api = (struct rtc_driver_api *)dev->api;

	if (api == NULL) {
		return -ENOSYS;
	}

	return api->wakeup_timer_cancel(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/rtc.h>

#endif /* ZEPHYR_INCLUDE_RTC_H_ */
