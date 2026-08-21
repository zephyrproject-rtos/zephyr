/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <zephyr/drivers/rtc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/sys/timeutil.h>

#include "bmc_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

#if DT_HAS_ALIAS(bmc_rtc)
#define RTC_NODE DT_ALIAS(bmc_rtc)
#else
#define RTC_NODE DT_NODELABEL(rtc)
#endif

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(RTC_NODE),
	     "CONFIG_BMC_RTC needs an enabled bmc-rtc devicetree alias");

static const struct device *const rtc_dev = DEVICE_DT_GET(RTC_NODE);

static int rtc_set_from_timespec(const struct timespec *ts)
{
	struct rtc_time tm;
	time_t t;
	int ret;

	if (!device_is_ready(rtc_dev)) {
		LOG_ERR("RTC device not ready");
		return -ENODEV;
	}

	t = ts->tv_sec;
	gmtime_r(&t, rtc_time_to_tm(&tm));
	tm.tm_nsec = ts->tv_nsec;

	ret = rtc_set_time(rtc_dev, &tm);
	if (ret < 0) {
		LOG_ERR("Could not set the RTC (err=%d)", ret);
		return ret;
	}

	LOG_INF("RTC: %04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1,
		tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	return 0;
}

int bmc_rtc_set_from_clock(void)
{
	struct timespec ts;
	int ret;

	ret = sys_clock_gettime(SYS_CLOCK_REALTIME, &ts);
	if (ret != 0) {
		LOG_ERR("sys_clock_gettime() failed (err=%d)", ret);
		return ret;
	}

	return rtc_set_from_timespec(&ts);
}

static int time_iso_to_ts(const char *str, struct timespec *ts)
{
	struct tm tm;
	int year, month, day, hour, minute, second, frac, ms;
	time_t epoch_sec;
	int ret;

	memset(&tm, 0, sizeof(tm));

	ret = sscanf(str, "%d-%d-%dT%d:%d:%d.%3dZ", &year, &month, &day, &hour, &minute, &second,
		     &frac);
	if (ret != 7) {
		return -EINVAL;
	}

	if (year < 1900 || month < 1) {
		return -EINVAL;
	}

	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = minute;
	tm.tm_sec = second;

	epoch_sec = timeutil_timegm(&tm);
	if (epoch_sec == -1) {
		return -EINVAL;
	}

	/* Normalise a 1 to 3 digit fraction of a second to milliseconds. */
	if (frac < 10) {
		ms = frac * 100;
	} else if (frac < 100) {
		ms = frac * 10;
	} else {
		ms = frac;
	}

	ts->tv_sec = epoch_sec;
	ts->tv_nsec = (int64_t)ms * NSEC_PER_MSEC;

	return 0;
}

int bmc_time_set_from_iso_str(const char *str)
{
	struct timespec ts;
	int ret;

	ret = time_iso_to_ts(str, &ts);
	if (ret < 0) {
		LOG_ERR("Could not parse the time string \"%s\" (err=%d)", str, ret);
		return ret;
	}

	ret = rtc_set_from_timespec(&ts);
	if (ret < 0) {
		return ret;
	}

	ret = sys_clock_settime(SYS_CLOCK_REALTIME, &ts);
	if (ret != 0) {
		LOG_ERR("sys_clock_settime() failed (err=%d)", ret);
		return ret;
	}

	return 0;
}

static int bmc_rtc_init(void)
{
	struct rtc_time tm;
	struct timespec ts;
	int ret;

	if (!device_is_ready(rtc_dev)) {
		LOG_WRN("RTC device not ready");
		return -ENODEV;
	}

	ret = rtc_get_time(rtc_dev, &tm);
	if (ret == -ENODATA) {
		LOG_INF("RTC is uninitialised, setting it to 1 Jan %d",
			CONFIG_BMC_RTC_DEFAULT_YEAR);

		memset(&tm, 0, sizeof(tm));
		tm.tm_year = CONFIG_BMC_RTC_DEFAULT_YEAR - 1900;
		tm.tm_mday = 1;

		ret = rtc_set_time(rtc_dev, &tm);
		if (ret < 0) {
			LOG_ERR("Could not set the RTC (err=%d)", ret);
			return ret;
		}

		ret = rtc_get_time(rtc_dev, &tm);
	}

	if (ret < 0) {
		LOG_ERR("Could not read the RTC (err=%d)", ret);
		return ret;
	}

	LOG_INF("RTC: %04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1,
		tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	ts.tv_sec = timeutil_timegm(rtc_time_to_tm(&tm));
	if (ts.tv_sec == -1) {
		LOG_ERR("Could not convert the RTC time to a UNIX timestamp");
		return -EINVAL;
	}

	ts.tv_nsec = tm.tm_nsec;

	ret = sys_clock_settime(SYS_CLOCK_REALTIME, &ts);
	if (ret != 0) {
		LOG_ERR("sys_clock_settime() failed (err=%d)", ret);
		return ret;
	}

	return 0;
}

BMC_COMPONENT_DEFINE(bmc_rtc, BMC_INIT_PHASE_PLATFORM, bmc_rtc_init, true);
