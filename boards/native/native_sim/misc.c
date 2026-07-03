/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NATIVE_SIM_SET_RTC_AT_BOOT)
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <time.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#endif /* CONFIG_NATIVE_SIM_SET_RTC_AT_BOOT */

#include "posix_native_task.h"
#include "nsi_timer_model.h"

#if defined(CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME)

static void set_realtime_default(void)
{
	hwtimer_set_real_time_mode(true);
}

NATIVE_TASK(set_realtime_default, PRE_BOOT_1, 0);

#endif /* CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME */

#if defined(CONFIG_NATIVE_SIM_SET_RTC_AT_BOOT)
static int set_rtc_at_boot(void)
{
	const struct device *const rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));
	struct rtc_time z_time;
	struct tm tm_time;
	time_t epoch;
	uint32_t nsec;
	uint64_t sec;

	if (!device_is_ready(rtc)) {
		return -ENODEV;
	}

	hwtimer_get_pseudohost_rtc_time(&nsec, &sec);
	epoch = (time_t)sec;

	if (localtime_r(&epoch, &tm_time) == NULL) {
		return -EINVAL;
	}

	z_time.tm_nsec = (int)nsec;
	z_time.tm_sec = tm_time.tm_sec;
	z_time.tm_min = tm_time.tm_min;
	z_time.tm_hour = tm_time.tm_hour;
	z_time.tm_mday = tm_time.tm_mday;
	z_time.tm_mon = tm_time.tm_mon;
	z_time.tm_year = tm_time.tm_year;
	z_time.tm_wday = tm_time.tm_wday;
	z_time.tm_yday = tm_time.tm_yday;
	z_time.tm_isdst = tm_time.tm_isdst;

	return rtc_set_time(rtc, &z_time);
}

SYS_INIT(set_rtc_at_boot, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY);

#endif /* CONFIG_NATIVE_SIM_SET_RTC_AT_BOOT */
