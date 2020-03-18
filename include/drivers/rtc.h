/*
 * Copyright (c) 2020 Paratronic
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_H_

/**
 * @file
 * @brief Public RTC driver APIs
 */

#include <zephyr/types.h>
#include <time.h>
#include <device.h>

typedef int (*rtc_api_get_time_t)(struct device *dev,
				  struct timespec *tp);
typedef int (*rtc_api_set_time_t)(struct device *dev,
				  const struct timespec *tp);

__subsystem struct rtc_driver_api {
	rtc_api_get_time_t get_time;
	rtc_api_set_time_t set_time;
};

/**
 * @brief Function to get RTC time
 *
 * @param[in] dev RTC device
 * @param[out] tp pointer to where to store the current rtc value
 *
 * @return 0 on success, negative on error
 */
__syscall int rtc_get_time(struct device *dev, struct timespec *tp);

static inline int z_impl_rtc_get_time(struct device *dev, struct timespec *tp)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->driver_api;

	return api->get_time(dev, tp);
}

/**
 * @brief Function to set RTC time
 *
 * @param[in] dev RTC device
 * @param[in] tp pointer to new rtc value
 *
 * @return 0 on success, negative on error
 */
__syscall int rtc_set_time(struct device *dev, const struct timespec *tp);

static inline int z_impl_rtc_set_time(struct device *dev,
				      const struct timespec *tp)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->driver_api;

	return api->set_time(dev, tp);
}

#include <syscalls/rtc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_H_ */
