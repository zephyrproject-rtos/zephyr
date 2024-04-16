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

/* Wed Dec 31 2025 23:59:55 GMT+0000 */
#define RTC_TEST_TIME_COUNTING_SET_TIME	  (1767225595UL)
#define RTC_TEST_TIME_COUNTING_LIMIT	  (RTC_TEST_TIME_COUNTING_SET_TIME + 10UL)
#define RTC_TEST_TIME_COUNTING_POLL_LIMIT (30U)

static const struct device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

ZTEST(rtc_api, test_time_counting)
{
	struct rtc_time datetime_set;
	struct rtc_time datetime_get;
	time_t timer_get;
	time_t timer_set = RTC_TEST_TIME_COUNTING_SET_TIME;
	time_t timer_get_last = timer_set;
	uint8_t i;

	gmtime_r(&timer_set, (struct tm *)(&datetime_set));

	zassert_equal(rtc_set_time(rtc, &datetime_set), 0, "Failed to set time");

	for (i = 0; i < RTC_TEST_TIME_COUNTING_POLL_LIMIT; i++) {
		/* Get time */
		zassert_equal(rtc_get_time(rtc, &datetime_get), 0, "Failed to get time");

		timer_get = timeutil_timegm((struct tm *)(&datetime_get));

		/* Validate if time incrementing */
		zassert_true(timer_get_last <= timer_get, "Time is decrementing");

		/* Check if limit reached */
		if (timer_get == RTC_TEST_TIME_COUNTING_LIMIT) {
			break;
		}

		/* Save last timer get */
		timer_get_last = timer_get;

		/* Limit polling rate */
		k_msleep(500);
	}

	zassert_true(i < RTC_TEST_TIME_COUNTING_POLL_LIMIT,
		     "Timeout occurred waiting for time to increment");
}
