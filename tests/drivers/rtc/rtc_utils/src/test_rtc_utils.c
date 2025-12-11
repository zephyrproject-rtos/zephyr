/*
 * # Copyright (c) 2024 Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "../../../../drivers/rtc/rtc_utils.h"

ZTEST(rtc_utils, test_rtc_utils_validate_rtc_time)
{
	/* Arbitrary out-out-range values. */
	const struct rtc_time alarm_time = {
		.tm_sec = 70,
		.tm_min = 70,
		.tm_hour = 25,
		.tm_mday = 35,
		.tm_mon = 15,
		.tm_year = 8000,
		.tm_wday = 8,
		.tm_yday = 370,
		.tm_nsec = INT32_MAX,
	};
	uint16_t masks[] = {RTC_ALARM_TIME_MASK_SECOND,  RTC_ALARM_TIME_MASK_MINUTE,
			    RTC_ALARM_TIME_MASK_HOUR,    RTC_ALARM_TIME_MASK_MONTHDAY,
			    RTC_ALARM_TIME_MASK_MONTH,   RTC_ALARM_TIME_MASK_YEAR,
			    RTC_ALARM_TIME_MASK_WEEKDAY, RTC_ALARM_TIME_MASK_YEARDAY,
			    RTC_ALARM_TIME_MASK_NSEC};
	ARRAY_FOR_EACH(masks, j)
	{
		bool ret = rtc_utils_validate_rtc_time(&alarm_time, masks[j]);

		zassert_false(ret, "RTC should reject invalid alarm time in field %zu.", j);
	}
}
