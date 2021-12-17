/**
 * @file
 * @brief RTC public API header file.
 */
/*
 * Copyright (c) 2021 LACROIX Group
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <device.h>
// #include <dt-bindings/rtc/rtc.h>

/**
 * @brief RTC driver APIs
 * @defgroup rtc_interface RTC driver APIs
 * @ingroup io_interfaces
 * @{
 */

/** @brief Wake up timers enum. */
enum rtc_wakeup_id {
	RTC_WUT0,       /**< Wake Up timer 0. */
	/* Add new item before RTC_WUT_NUM. */
	RTC_WUT_NUM,    /**< Number of wut for the driver. */
};

/** @brief Alarms enum. */
enum rtc_alarm_id {
	RTC_ALARMA,     /**< Alarm A. */
	RTC_ALARMB,     /**< Alarm B. */
	/* Add new item before RTC_ALARM_NUM. */
	RTC_ALARM_NUM,  /**< Number of alarms for the driver. */
};

/** @brief Specifies if the RTC Alarm is on date or Week day. */
enum rtc_alarm_date_weekday {
	RTC_ALARM_DATE_SEL,     /**< Alarm Date is selected, set parameter to a value in the 1-31 range. */
	RTC_ALARM_WEEKDAY_SEL,  /**< Alarm Weekday is selected, set parameter to a value in the 0-6 range (days since Sunday). */
};

/** @brief Specifies the RTC Alarm Masks. */
enum rtc_alarm_mask {
	RTC_ALARM_MASK_NONE,            /**< No mask. */
	RTC_ALARM_MASK_DATE_WEEKDAY,    /**< Mask on date or weekday. */
	RTC_ALARM_MASK_HOURS,           /**< Mask on hours. */
	RTC_ALARM_MASK_MIN,             /**< Mask on minutes. */
	RTC_ALARM_MASK_SEC,             /**< Mask on seconds. */
	RTC_ALARM_MASK_ALL,             /**< Mask on all. */
};

/** @brief Wake up timer structure. */
struct rtc_wakeup {
	uint32_t period;                                /**< Wake up timer period in milliseconds. */
	void (*callback)(const struct device *dev);     /**< Callback function. Can be NULL. */
};

/** @brief Alarm structure.
 * @param alarm_time Alarm time members
 * @param alarm_date_weekday_sel Alarm selection: on Date or on WeekDay.
 * 	If Alarm date is selected, use tm_mday [1 to 31] to set the alarm
 *  If WeekDay date is selected, use tm_wday [0 to 6] to set the alarm
 * @param callback	Callback function. Can be NULL.
 */
struct rtc_alarm {
	struct tm alarm_time;           /**< Alarm time members. */
	enum rtc_alarm_mask alarm_mask; /**< Alarm mask. */
	/** Alarm selection: on Date or on WeekDay.
	 * If Alarm date is selected, use tm_mday [1 to 31] to set the alarm
	 * If WeekDay date is selected, use tm_wday [0 to 6] to set the alarm
	 */
	enum rtc_alarm_date_weekday alarm_date_weekday_sel;
	void (*callback)(const struct device *dev);     /**< Callback function. Can be NULL. */
};

/**
 * @brief RTC driver API
 *
 * This is the mandatory API any RTC driver needs to expose.
 */
__subsystem struct rtc_driver_api {
	int (*set_time)(const struct device *dev, const struct tm date_time);
	int (*get_time)(const struct device *dev, struct tm *date_time);
	int (*set_wakeup_timer)(const struct device *dev, struct rtc_wakeup wut, enum rtc_wakeup_id wut_id);
	int (*start_wakeup_timer)(const struct device *dev, enum rtc_wakeup_id wut_id);
	int (*stop_wakeup_timer)(const struct device *dev, enum rtc_wakeup_id wut_id);
	int (*set_alarm)(const struct device *dev, struct rtc_alarm alarm, enum rtc_alarm_id alarm_id);
	int (*start_alarm)(const struct device *dev, enum rtc_alarm_id alarm_id);
	int (*stop_alarm)(const struct device *dev, enum rtc_alarm_id alarm_id);
};

/**
 * @brief Set current date and time
 *
 * @param dev Pointeur to the device instance
 * @param date_time Structure with date and time structure.
 * @retval 0		On success.
 * @retval -ENOTSUP	If request is not supported
 * @retval -EINVAL	If date or time settings are invalid.
 */
__syscall int        rtc_set_time(const struct device *dev, const struct tm date_time);
static inline int z_impl_rtc_set_time(const struct device *dev, const struct tm date_time)
{
	const struct rtc_driver_api *api = dev->api;

	__ASSERT(api->print, "Callback pointer should not be NULL");
	return (api->set_time(dev, date_time));
}

/**
 * @brief Get current date and time
 *
 * @param dev Pointeur to the device instance
 * @param date_time Pointeur to date and time structure.
 *
 * @retval 0		On success.
 * @retval -ENOTSUP	If request is not supported.
 */
__syscall int        rtc_get_time(const struct device *dev, struct tm *date_time);
static inline int z_impl_rtc_get_time(const struct device *dev, struct tm *date_time)
{
	const struct rtc_driver_api *api = dev->api;

	__ASSERT(api->print, "Callback pointer should not be NULL");
	return (api->get_time(dev, date_time));
}

/**
 * @brief Set wake up timer
 *
 * @param dev Pointeur to the device instance
 * @param wut Wake up timer parameters
 * @param wut_id Wake up timer id
 *
 * @retval 0		On success.
 * @retval -ENOTSUP	If request is not supported.
 * @retval -EINVAL	If wut_id is invalid.
 */
__syscall int        rtc_set_wakeup_timer(const struct device *dev, struct rtc_wakeup wut, enum rtc_wakeup_id wut_id);
static inline int z_impl_rtc_set_wakeup_timer(const struct device *dev, struct rtc_wakeup wut, enum rtc_wakeup_id wut_id)
{
	const struct rtc_driver_api *api = dev->api;

	__ASSERT(api->print, "Callback pointer should not be NULL");
	return (api->set_wakeup_timer(dev, wut, wut_id));
}

/**
 * @brief Start wake up timer with interrupt
 *
 * @param dev Pointeur to the device instance
 * @param wut_id Wake up timer id
 *
 * @retval 0		On success.
 * @retval -ENOTSUP	If request is not supported.
 * @retval -EINVAL	If wut_id is invalid.
 */
__syscall int        rtc_start_wakeup_timer(const struct device *dev, enum rtc_wakeup_id wut_id);
static inline int z_impl_rtc_start_wakeup_timer(const struct device *dev, enum rtc_wakeup_id wut_id)
{
	const struct rtc_driver_api *api = dev->api;

	__ASSERT(api->print, "Callback pointer should not be NULL");
	return (api->start_wakeup_timer(dev, wut_id));
}

/**
 * @brief Stop wake up timer
 *
 * @param dev Pointeur to the device instance
 * @param wut_id Wake up timer id
 *
 * @retval 0		On success.
 * @retval -ENOTSUP	If request is not supported.
 * @retval -EINVAL	If wut_id is invalid.
 */
__syscall int        rtc_stop_wakeup_timer(const struct device *dev, enum rtc_wakeup_id wut_id);
static inline int z_impl_rtc_stop_wakeup_timer(const struct device *dev, enum rtc_wakeup_id wut_id)
{
	const struct rtc_driver_api *api = dev->api;

	__ASSERT(api->print, "Callback pointer should not be NULL");
	return (api->stop_wakeup_timer(dev, wut_id));
}

/**
 * @brief Set alarm
 *
 * @param dev Pointeur to the device instance
 * @param alarm Alarm parameters
 * @param alarm_id Alarm id
 *
 * @retval 0		On success.
 * @retval -ENOTSUP	If request is not supported.
 * @retval -EINVAL	If alarm_id is invalid.
 */
__syscall int        rtc_set_alarm(const struct device *dev, struct rtc_alarm alarm, enum rtc_alarm_id alarm_id);
static inline int z_impl_rtc_set_alarm(const struct device *dev, struct rtc_alarm alarm, enum rtc_alarm_id alarm_id)
{
	const struct rtc_driver_api *api = dev->api;

	__ASSERT(api->print, "Callback pointer should not be NULL");
	return (api->set_alarm(dev, alarm, alarm_id));
}

/**
 * @brief Start alarm
 *
 * @param dev Pointeur to the device instance
 * @param alarm_id Alarm id
 *
 * @retval 0		On success.
 * @retval -ENOTSUP	If request is not supported.
 * @retval -EINVAL	If alarm_id is invalid.
 */
__syscall int        rtc_start_alarm(const struct device *dev, enum rtc_alarm_id alarm_id);
static inline int z_impl_rtc_start_alarm(const struct device *dev, enum rtc_alarm_id alarm_id)
{
	const struct rtc_driver_api *api = dev->api;

	__ASSERT(api->print, "Callback pointer should not be NULL");
	return (api->start_alarm(dev, alarm_id));
}

/**
 * @brief Stop alarm
 *
 * @param dev Pointeur to the device instance
 * @param alarm_id Alarm id
 *
 * @retval 0		On success.
 * @retval -ENOTSUP	If request is not supported.
 * @retval -EINVAL	If alarm_id is invalid.
 */
__syscall int        rtc_stop_alarm(const struct device *dev, enum rtc_alarm_id alarm_id);
static inline int z_impl_rtc_stop_alarm(const struct device *dev, enum rtc_alarm_id alarm_id)
{
	const struct rtc_driver_api *api = dev->api;

	__ASSERT(api->print, "Callback pointer should not be NULL");
	return (api->stop_alarm(dev, alarm_id));
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/rtc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_H_ */
