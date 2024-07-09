/*
 * Copyright (c) 2023 Trackunit Corporation
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file drivers/rtc.h
 *
 * @brief Inline syscall implementations for RTC (real time clock) API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_H_
#error "Should only be included by zephyr/drivers/rtc.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_INTERNAL_RTC_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_INTERNAL_RTC_IMPL_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_rtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	return api->set_time(dev, timeptr);
}

static inline int z_impl_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	return api->get_time(dev, timeptr);
}

#if defined(CONFIG_RTC_ALARM)

static inline int z_impl_rtc_alarm_get_supported_fields(const struct device *dev, uint16_t id,
							uint16_t *mask)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->alarm_get_supported_fields == NULL) {
		return -ENOSYS;
	}

	return api->alarm_get_supported_fields(dev, id, mask);
}

static inline int z_impl_rtc_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
					    const struct rtc_time *timeptr)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->alarm_set_time == NULL) {
		return -ENOSYS;
	}

	return api->alarm_set_time(dev, id, mask, timeptr);
}

static inline int z_impl_rtc_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
					    struct rtc_time *timeptr)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->alarm_get_time == NULL) {
		return -ENOSYS;
	}

	return api->alarm_get_time(dev, id, mask, timeptr);
}

static inline int z_impl_rtc_alarm_is_pending(const struct device *dev, uint16_t id)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->alarm_is_pending == NULL) {
		return -ENOSYS;
	}

	return api->alarm_is_pending(dev, id);
}

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

#if defined(CONFIG_RTC_UPDATE)

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

#if defined(CONFIG_RTC_CALIBRATION)

static inline int z_impl_rtc_set_calibration(const struct device *dev, int32_t calibration)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->set_calibration == NULL) {
		return -ENOSYS;
	}

	return api->set_calibration(dev, calibration);
}

static inline int z_impl_rtc_get_calibration(const struct device *dev, int32_t *calibration)
{
	const struct rtc_driver_api *api = (const struct rtc_driver_api *)dev->api;

	if (api->get_calibration == NULL) {
		return -ENOSYS;
	}

	return api->get_calibration(dev, calibration);
}

#endif /* CONFIG_RTC_CALIBRATION */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_INTERNAL_RTC_IMPL_H_ */
