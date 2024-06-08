/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/realtime.h>

BUILD_ASSERT(
	CONFIG_RTC_HWCLOCK_INIT_PRIORITY > CONFIG_RTC_INIT_PRIORITY,
	"Hardware clock init prio must be higher than the RTC device driver"
);

static const struct device *rtc = DEVICE_DT_GET(DT_CHOSEN(zephyr_rtc));

static const struct sys_datetime *rtc_time_to_sys_datetime(const struct rtc_time *timeptr)
{
	return (const struct sys_datetime *)timeptr;
}

static int rtc_hwclock_init(void)
{
	int ret;
	struct rtc_time rtctime;
	const struct sys_datetime *datetime;

	if (!device_is_ready(rtc)) {
		return -ENODEV;
	}

	ret = rtc_get_time(rtc, &rtctime);
	if (ret == -ENODATA) {
		return 0;
	} else if (ret < 0) {
		return ret;
	}

	datetime = rtc_time_to_sys_datetime(&rtctime);
	return sys_realtime_set_datetime(datetime);
}

SYS_INIT(rtc_hwclock_init, POST_KERNEL, CONFIG_RTC_HWCLOCK_INIT_PRIORITY);
