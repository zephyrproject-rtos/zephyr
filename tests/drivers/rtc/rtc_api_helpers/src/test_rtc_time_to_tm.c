/*
 * Copyright 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/rtc.h>
#include <time.h>

ZTEST(rtc_api_helpers, validate_rtc_time_compat_with_tm)
{
	zassert(offsetof(struct rtc_time, tm_sec) == offsetof(struct tm, tm_sec),
		"Offset of tm_sec in struct rtc_time does not match struct tm");

	zassert(offsetof(struct rtc_time, tm_min) == offsetof(struct tm, tm_min),
		"Offset of tm_min in struct rtc_time does not match struct tm");

	zassert(offsetof(struct rtc_time, tm_hour) == offsetof(struct tm, tm_hour),
		"Offset of tm_hour in struct rtc_time does not match struct tm");

	zassert(offsetof(struct rtc_time, tm_mday) == offsetof(struct tm, tm_mday),
		"Offset of tm_mday in struct rtc_time does not match struct tm");

	zassert(offsetof(struct rtc_time, tm_mon) == offsetof(struct tm, tm_mon),
		"Offset of tm_mon in struct rtc_time does not match struct tm");

	zassert(offsetof(struct rtc_time, tm_year) == offsetof(struct tm, tm_year),
		"Offset of tm_year in struct rtc_time does not match struct tm");

	zassert(offsetof(struct rtc_time, tm_wday) == offsetof(struct tm, tm_wday),
		"Offset of tm_wday in struct rtc_time does not match struct tm");

	zassert(offsetof(struct rtc_time, tm_yday) == offsetof(struct tm, tm_yday),
		"Offset of tm_yday in struct rtc_time does not match struct tm");

	zassert(offsetof(struct rtc_time, tm_isdst) == offsetof(struct tm, tm_isdst),
		"Offset of tm_isdts in struct rtc_time does not match struct tm");
}

ZTEST(rtc_api_helpers, validate_rtc_time_to_tm)
{
	struct rtc_time rtc_datetime;
	struct tm *datetime = NULL;

	datetime = rtc_time_to_tm(&rtc_datetime);

	zassert(((struct tm *)&rtc_datetime) == datetime, "Incorrect typecast");
}
