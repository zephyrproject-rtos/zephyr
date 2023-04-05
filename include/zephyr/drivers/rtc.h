/*
 * Copyright (c) 2023 Trackunit Corporation
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file drivers/rtc.h
 * @brief Public real time clock driver API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_H_

/**
 * @brief RTC Interface
 * @defgroup rtc_interface RTC Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mask for alarm time fields to enable when setting alarm time
 * @name RTC Alarm Time Mask
 * @anchor RTC_ALARM_TIME_MASK
 * @{
 */
#define RTC_ALARM_TIME_MASK_SECOND	BIT(0)
#define RTC_ALARM_TIME_MASK_MINUTE	BIT(1)
#define RTC_ALARM_TIME_MASK_HOUR	BIT(2)
#define RTC_ALARM_TIME_MASK_MONTHDAY	BIT(3)
#define RTC_ALARM_TIME_MASK_MONTH	BIT(4)
#define RTC_ALARM_TIME_MASK_YEAR	BIT(5)
#define RTC_ALARM_TIME_MASK_WEEKDAY	BIT(6)
#define RTC_ALARM_TIME_MASK_YEARDAY	BIT(7)
#define RTC_ALARM_TIME_MASK_NSEC	BIT(8)
/**
 * @}
 */

/**
 * @brief Structure for storing date and time values with sub-second precision.
 *
 * @details The structure is 1-1 mapped to the struct tm for the members
 * \p tm_sec to \p tm_isdst making it compatible with the standard time library.
 *
 * @note Use \ref rtc_time_to_tm() to safely cast from a \ref rtc_time
 * pointer to a \ref tm pointer.
 */
struct rtc_time {
	int tm_sec;	/**< Seconds [0, 59] */
	int tm_min;	/**< Minutes [0, 59] */
	int tm_hour;	/**< Hours [0, 23] */
	int tm_mday;	/**< Day of the month [1, 31] */
	int tm_mon;	/**< Month [0, 11] */
	int tm_year;	/**< Year - 1900 */
	int tm_wday;	/**< Day of the week [0, 6] (Sunday = 0) (Unknown = -1) */
	int tm_yday;	/**< Day of the year [0, 365] (Unknown = -1) */
	int tm_isdst;	/**< Daylight saving time flag [-1] (Unknown = -1) */
	int tm_nsec;	/**< Nanoseconds [0, 999999999] (Unknown = 0) */
};

/**
 * @typedef rtc_update_callback
 * @brief RTC update event callback
 *
 * @param dev Device instance invoking the handler
 * @param user_data Optional user data provided when update irq callback is set
 */
typedef void (*rtc_update_callback)(const struct device *dev, void *user_data);

/**
 * @typedef rtc_alarm_callback
 * @brief RTC alarm triggered callback
 *
 * @param dev Device instance invoking the handler
 * @param id Alarm id
 * @param user_data Optional user data passed with the alarm configuration
 */
typedef void (*rtc_alarm_callback)(const struct device *dev, uint16_t id, void *user_data);

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

/**
 * @typedef rtc_api_set_time
 * @brief API for setting RTC time
 */
typedef int (*rtc_api_set_time)(const struct device *dev, const struct rtc_time *timeptr);

/**
 * @typedef rtc_api_get_time
 * @brief API for getting RTC time
 */
typedef int (*rtc_api_get_time)(const struct device *dev, struct rtc_time *timeptr);

/**
 * @typedef rtc_api_alarm_get_supported_fields
 * @brief API for getting the supported fields of the RTC alarm time
 */
typedef int (*rtc_api_alarm_get_supported_fields)(const struct device *dev, uint16_t id,
						  uint16_t *mask);

/**
 * @typedef rtc_api_alarm_set_time
 * @brief API for setting RTC alarm time
 */
typedef int (*rtc_api_alarm_set_time)(const struct device *dev, uint16_t id, uint16_t mask,
				      const struct rtc_time *timeptr);

/**
 * @typedef rtc_api_alarm_get_time
 * @brief API for getting RTC alarm time
 */
typedef int (*rtc_api_alarm_get_time)(const struct device *dev, uint16_t id, uint16_t *mask,
				      struct rtc_time *timeptr);

/**
 * @typedef rtc_api_alarm_is_pending
 * @brief API for testing if RTC alarm is pending
 */
typedef int (*rtc_api_alarm_is_pending)(const struct device *dev, uint16_t id);

/**
 * @typedef rtc_api_alarm_set_callback
 * @brief API for setting RTC alarm callback
 */
typedef int (*rtc_api_alarm_set_callback)(const struct device *dev, uint16_t id,
					  rtc_alarm_callback callback, void *user_data);

/**
 * @typedef rtc_api_update_set_callback
 * @brief API for setting RTC update callback
 */
typedef int (*rtc_api_update_set_callback)(const struct device *dev,
					   rtc_update_callback callback, void *user_data);

/**
 * @typedef rtc_api_set_calibration
 * @brief API for setting RTC calibration
 */
typedef int (*rtc_api_set_calibration)(const struct device *dev, int32_t calibration);

/**
 * @typedef rtc_api_get_calibration
 * @brief API for getting RTC calibration
 */
typedef int (*rtc_api_get_calibration)(const struct device *dev, int32_t *calibration);

/**
 * @brief RTC driver API
 */
__subsystem struct rtc_driver_api {
	rtc_api_set_time set_time;
	rtc_api_get_time get_time;
#if defined(CONFIG_RTC_ALARM) || defined(__DOXYGEN__)
	rtc_api_alarm_get_supported_fields alarm_get_supported_fields;
	rtc_api_alarm_set_time alarm_set_time;
	rtc_api_alarm_get_time alarm_get_time;
	rtc_api_alarm_is_pending alarm_is_pending;
	rtc_api_alarm_set_callback alarm_set_callback;
#endif /* CONFIG_RTC_ALARM */
#if defined(CONFIG_RTC_UPDATE) || defined(__DOXYGEN__)
	rtc_api_update_set_callback update_set_callback;
#endif /* CONFIG_RTC_UPDATE */
#if defined(CONFIG_RTC_CALIBRATION) || defined(__DOXYGEN__)
	rtc_api_set_calibration set_calibration;
	rtc_api_get_calibration get_calibration;
#endif /* CONFIG_RTC_CALIBRATION */
};

/** @endcond */

/**
 * @brief API for setting RTC time.
 *
 * @param dev Device instance
 * @param timeptr The time to set
 *
 * @return 0 if successful
 * @return -EINVAL if RTC time is invalid or exceeds hardware capabilities
 * @return -errno code if failure
 */
__syscall int rtc_set_time(const struct device *dev, const struct rtc_time *timeptr);

static inline int z_impl_rtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	return api->set_time(dev, timeptr);
}

/**
 * @brief API for getting RTC time.
 *
 * @param dev Device instance
 * @param timeptr Destination for the time
 *
 * @return 0 if successful
 * @return -ENODATA if RTC time has not been set
 * @return -errno code if failure
 */
__syscall int rtc_get_time(const struct device *dev, struct rtc_time *timeptr);

static inline int z_impl_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	return api->get_time(dev, timeptr);
}

/**
 * @name RTC Interface Alarm
 * @{
 */
#if defined(CONFIG_RTC_ALARM) || defined(__DOXYGEN__)

/**
 * @brief API for getting the supported fields of the RTC alarm time.
 *
 * @param dev Device instance
 * @param id Id of the alarm
 * @param mask Mask of fields in the alarm time which are supported
 *
 * @note Bits in the mask param are defined here @ref RTC_ALARM_TIME_MASK.
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range or time is invalid
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_get_supported_fields(const struct device *dev, uint16_t id,
					     uint16_t *mask);

static inline int z_impl_rtc_alarm_get_supported_fields(const struct device *dev, uint16_t id,
							uint16_t *mask)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->alarm_get_supported_fields == NULL) {
		return -ENOSYS;
	}

	return api->alarm_get_supported_fields(dev, id, mask);
}

/**
 * @brief API for setting RTC alarm time.
 *
 * @details To enable an RTC alarm, one or more fields of the RTC alarm time
 * must be enabled. The mask designates which fields of the RTC alarm time to
 * enable. If the mask parameter is 0, the alarm will be disabled. The RTC
 * alarm will trigger when all enabled fields of the alarm time match the RTC
 * time.
 *
 * @param dev Device instance
 * @param id Id of the alarm
 * @param mask Mask of fields in the alarm time to enable
 * @param timeptr The alarm time to set
 *
 * @note The timeptr param may be NULL if the mask param is 0
 * @note Only the enabled fields in the timeptr param need to be configured
 * @note Bits in the mask param are defined here @ref RTC_ALARM_TIME_MASK
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range or time is invalid
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				 const struct rtc_time *timeptr);

static inline int z_impl_rtc_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
					    const struct rtc_time *timeptr)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->alarm_set_time == NULL) {
		return -ENOSYS;
	}

	return api->alarm_set_time(dev, id, mask, timeptr);
}

/**
 * @brief API for getting RTC alarm time.
 *
 * @param dev Device instance
 * @param id Id of the alarm
 * @param mask Destination for mask of fields which are enabled in the alarm time
 * @param timeptr Destination for the alarm time
 *
 * @note Bits in the mask param are defined here @ref RTC_ALARM_TIME_MASK
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				 struct rtc_time *timeptr);

static inline int z_impl_rtc_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
					    struct rtc_time *timeptr)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->alarm_get_time == NULL) {
		return -ENOSYS;
	}

	return api->alarm_get_time(dev, id, mask, timeptr);
}

/**
 * @brief API for testing if RTC alarm is pending.
 *
 * @details Test whether or not the alarm with id is pending. If the alarm
 * is pending, the pending status is cleared.
 *
 * @param dev Device instance
 * @param id Id of the alarm to test
 *
 * @return 1 if alarm was pending
 * @return 0 if alarm was not pending
 * @return -EINVAL if id is out of range
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_is_pending(const struct device *dev, uint16_t id);

static inline int z_impl_rtc_alarm_is_pending(const struct device *dev, uint16_t id)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->alarm_is_pending == NULL) {
		return -ENOSYS;
	}

	return api->alarm_is_pending(dev, id);
}

/**
 * @brief API for setting alarm callback.
 *
 * @details Setting the alarm callback for an alarm, will enable the
 * alarm callback. When the callback for an alarm is enabled, the
 * alarm triggered event will invoke the callback, after which the
 * alarm pending status will be cleared automatically. The alarm will
 * remain enabled until manually disabled using
 * \ref rtc_alarm_set_time().
 *
 * To disable the alarm callback for an alarm, the \p callback and
 * \p user_data parameters must be set to NULL. When the alarm
 * callback for an alarm is disabled, the alarm triggered event will
 * set the alarm status to "pending". To check if the alarm status is
 * "pending", use \ref rtc_alarm_is_pending().
 *
 * @param dev Device instance
 * @param id Id of the alarm for which the callback shall be set
 * @param callback Callback called when alarm occurs
 * @param user_data Optional user data passed to callback
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_set_callback(const struct device *dev, uint16_t id,
				     rtc_alarm_callback callback, void *user_data);

static inline int z_impl_rtc_alarm_set_callback(const struct device *dev, uint16_t id,
						rtc_alarm_callback callback, void *user_data)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->alarm_set_callback == NULL) {
		return -ENOSYS;
	}

	return api->alarm_set_callback(dev, id, callback, user_data);
}

#endif /* CONFIG_RTC_ALARM */
/**
 * @}
 */

/**
 * @name RTC Interface Update
 * @{
 */
#if defined(CONFIG_RTC_UPDATE) || defined(__DOXYGEN__)

/**
 * @brief API for setting update callback.
 *
 * @details Setting the update callback will enable the update
 * callback. The update callback will be invoked every time the
 * RTC clock is updated by 1 second. It can be used to
 * synchronize the RTC clock with other clock sources.
 *
 * To disable the update callback for the RTC clock, the
 * \p callback and \p user_data parameters must be set to NULL.
 *
 * @param dev Device instance
 * @param callback Callback called when update occurs
 * @param user_data Optional user data passed to callback
 *
 * @return 0 if successful
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_update_set_callback(const struct device *dev, rtc_update_callback callback,
				      void *user_data);

static inline int z_impl_rtc_update_set_callback(const struct device *dev,
						 rtc_update_callback callback, void *user_data)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->update_set_callback == NULL) {
		return -ENOSYS;
	}

	return api->update_set_callback(dev, callback, user_data);
}

#endif /* CONFIG_RTC_UPDATE */
/**
 * @}
 */

/**
 * @name RTC Interface Calibration
 * @{
 */
#if defined(CONFIG_RTC_CALIBRATION) || defined(__DOXYGEN__)

/**
 * @brief API for setting RTC calibration.
 *
 * @details Calibration is applied to the RTC clock input. A
 * positive calibration value will increase the frequency of
 * the RTC clock, a negative value will decrease the
 * frequency of the RTC clock.
 *
 * @param dev Device instance
 * @param calibration Calibration to set in parts per billion
 *
 * @return 0 if successful
 * @return -EINVAL if calibration is out of range
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_set_calibration(const struct device *dev, int32_t calibration);

static inline int z_impl_rtc_set_calibration(const struct device *dev, int32_t calibration)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->set_calibration == NULL) {
		return -ENOSYS;
	}

	return api->set_calibration(dev, calibration);
}

/**
 * @brief API for getting RTC calibration.
 *
 * @param dev Device instance
 * @param calibration Destination for calibration in parts per billion
 *
 * @return 0 if successful
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_get_calibration(const struct device *dev, int32_t *calibration);

static inline int z_impl_rtc_get_calibration(const struct device *dev, int32_t *calibration)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->get_calibration == NULL) {
		return -ENOSYS;
	}

	return api->get_calibration(dev, calibration);
}

#endif /* CONFIG_RTC_CALIBRATION */
/**
 * @}
 */

/**
 * @name RTC Interface Helpers
 * @{
 */

/**
 * @brief Forward declaration of struct tm for \ref rtc_time_to_tm().
 */
struct tm;

/**
 * @brief Convenience function for safely casting a \ref rtc_time pointer
 * to a \ref tm pointer.
 */
static inline struct tm *rtc_time_to_tm(struct rtc_time *timeptr)
{
	return (struct tm *)timeptr;
}

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/rtc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_H_ */
