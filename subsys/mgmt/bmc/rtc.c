/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(bmc_rtc, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/timeutil.h>

#include "rtc.h"

int rtc_set_from_timespec(const struct timespec *ts)
{
	static const struct device *rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));
	static struct rtc_time tm;
	time_t t;
	int ret;

	if (!device_is_ready(rtc)) {
		LOG_ERR("No RTC device");
		return -ENODEV;
	}

	t = ts->tv_sec;
	gmtime_r(&t, rtc_time_to_tm(&tm));
	tm.tm_nsec = ts->tv_nsec;

	ret = rtc_set_time(rtc, &tm);
	if (ret < 0) {
		LOG_ERR("Can not set RTC (err=%d)", ret);
		return ret;
	}

	LOG_INF("RTC: %04d-%02d-%02d %02d:%02d:%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	return 0;
}

int rtc_set_from_clock(void)
{
	static struct timespec ts;
	int ret;

	ret = sys_clock_gettime(SYS_CLOCK_REALTIME, &ts);
	if (ret != 0) {
		LOG_ERR("sys_clock_gettime failed (err=%d)", ret);
		return ret;
	}

	ret = rtc_set_from_timespec(&ts);
	if (ret != 0) {
		LOG_ERR("rtc_set_from_timespec failed (err=%d)", ret);
		return ret;
	}

	return 0;
}

static int time_iso_to_ts(const char *str, struct timespec *ts)
{
	struct tm tm;
	int year, month, day, hour, minute, second, frac, ms;
	time_t epoch_sec;
	int ret;

	memset(&tm, 0, sizeof(struct tm));

	ret = sscanf(str, "%d-%d-%dT%d:%d:%d.%3dZ",
			&year, &month, &day, &hour, &minute, &second, &frac);
	if (ret != 7)
		return -EINVAL;
	if (year < 1900)
		return -EINVAL;
	if (month < 1)
		return -EINVAL;

	tm.tm_year = year - 1900;
	tm.tm_mon  = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min  = minute;
	tm.tm_sec  = second;

	epoch_sec = timeutil_timegm(&tm);
	if (epoch_sec == -1)
		return -EINVAL;

	if (frac < 10)
		ms = frac * 100;
	else if (frac < 100)
		ms = frac * 10;
	else
		ms = frac;

	ts->tv_sec = epoch_sec;
	ts->tv_nsec = (int64_t)ms * NSEC_PER_MSEC;

	return 0;
}

int time_set_from_iso_str(const char *str)
{
	static struct timespec ts;
	int ret;

	ret = time_iso_to_ts(str, &ts);
	if (ret != 0) {
		LOG_ERR("time_iso_to_ts failed (err=%d)", ret);
		return ret;
	}

	ret = rtc_set_from_timespec(&ts);
	if (ret != 0) {
		LOG_ERR("rtc_set_from_timespec failed (err=%d)", ret);
		return ret;
	}

	ret = sys_clock_settime(SYS_CLOCK_REALTIME, &ts);
	if (ret != 0) {
		LOG_ERR("sys_clock_settime failed (err=%d)", ret);
		return ret;
	}

	return 0;
}

int rtc_init(void)
{
	static const struct device *rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));
	static struct rtc_time tm;
	static struct timespec ts;
	int ret;

	if (!device_is_ready(rtc)) {
		LOG_WRN("No RTC device");
		return 0;
	}

	ret = rtc_get_time(rtc, &tm);
	if (ret == -ENODATA) {
		LOG_INF("RTC is uninitialised, setting to 1 Jan 2026");
		memset(&tm, 0, sizeof(tm));
		tm.tm_year = 2026 - 1900;
		tm.tm_mday = 1;
		ret = rtc_set_time(rtc, &tm);
		if (ret < 0) {
			LOG_ERR("Can not set RTC (err=%d)", ret);
			return ret;
		}
		ret = rtc_get_time(rtc, &tm);
	}
	if (ret < 0) {
		LOG_ERR("Can not get RTC (err=%d)", ret);
		return ret;
	}

	LOG_INF("RTC: %04d-%02d-%02d %02d:%02d:%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	ts.tv_sec = timeutil_timegm(rtc_time_to_tm(&tm));
	if (ts.tv_sec == -1) {
		LOG_ERR("Failed to calculate UNIX time from RTC");
		return -EINVAL;
	}
	ts.tv_nsec = tm.tm_nsec;

	ret = sys_clock_settime(SYS_CLOCK_REALTIME, &ts);
	if (ret != 0) {
		LOG_ERR("sys_clock_settime failed (err=%d)", ret);
		return ret;
	}

	return ret;
}
