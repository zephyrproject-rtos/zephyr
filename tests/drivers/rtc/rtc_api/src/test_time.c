/*
 * Copyright 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/timeutil.h>

#include <time.h>
#include <string.h>

/* Wed Dec 31 2025 23:59:55 GMT+0000 */
#define RTC_TEST_GET_SET_TIME	  (1767225595UL)
#define RTC_TEST_GET_SET_TIME_TOL (1UL)

static const struct device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

ZTEST(rtc_api, test_set_get_time)
{
	struct rtc_time datetime_set;
	struct rtc_time datetime_get;
	time_t timer_get;
	time_t timer_set = RTC_TEST_GET_SET_TIME;

	gmtime_r(&timer_set, (struct tm *)(&datetime_set));

	datetime_set.tm_isdst = -1;
	datetime_set.tm_nsec = 0;

	memset(&datetime_get, 0xFF, sizeof(datetime_get));

	zassert_equal(rtc_set_time(rtc, &datetime_set), 0, "Failed to set time");

	zassert_equal(rtc_get_time(rtc, &datetime_get), 0,
		      "Failed to get time using rtc_time_get()");

	zassert_true((datetime_get.tm_sec > -1) && (datetime_get.tm_sec < 60),
		     "Invalid tm_sec");

	zassert_true((datetime_get.tm_min > -1) && (datetime_get.tm_min < 60),
		     "Invalid tm_min");

	zassert_true((datetime_get.tm_hour > -1) && (datetime_get.tm_hour < 24),
		     "Invalid tm_hour");

	zassert_true((datetime_get.tm_mday > 0) && (datetime_get.tm_mday < 32),
		      "Invalid tm_mday");

	zassert_true((datetime_get.tm_year > 124) && (datetime_get.tm_year < 127),
		     "Invalid tm_year");

	zassert_true((datetime_get.tm_wday > -2) && (datetime_get.tm_wday < 7),
		     "Invalid tm_wday");

	zassert_true((datetime_get.tm_yday > -2) && (datetime_get.tm_yday < 366),
		     "Invalid tm_yday");

	zassert_equal(datetime_get.tm_isdst, -1, "Invalid tm_isdst");

	zassert_true((datetime_get.tm_nsec > -1) && (datetime_get.tm_yday < 1000000000),
		     "Invalid tm_yday");

	timer_get = timeutil_timegm((struct tm *)(&datetime_get));

	zassert_true((timer_get >= RTC_TEST_GET_SET_TIME) &&
		     (timer_get <= (RTC_TEST_GET_SET_TIME + RTC_TEST_GET_SET_TIME_TOL)),
		     "Got unexpected time");
}
