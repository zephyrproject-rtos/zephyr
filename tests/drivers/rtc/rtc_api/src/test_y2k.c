/*
 * Copyright 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>

#include <time.h>

/* date "+%s" -d "Sat Jan  1 2000 00:00:00 GMT+0000" */
#define Y2K_STAMP 946684800UL

#define SECONDS_BEFORE 1
#define SECONDS_AFTER 1

#define RTC_TEST_START_TIME (Y2K_STAMP - SECONDS_BEFORE)
#define RTC_TEST_STOP_TIME (Y2K_STAMP + SECONDS_AFTER)

static const struct device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

ZTEST(rtc_api, test_y2k)
{
	enum test_time {
		Y99,
		Y2K,
	};

	static struct rtc_time rtm[2];
	struct tm *const tm[2] = {
		(struct tm *const)&rtm[0],
		(struct tm *const)&rtm[1],
	};
	const time_t t[] = {
		[Y99] = RTC_TEST_START_TIME,
		[Y2K] = RTC_TEST_STOP_TIME,
	};

	/* Party like it's 1999 */
	zassert_not_null(gmtime_r(&t[Y99], tm[Y99]));
	zassert_ok(rtc_set_time(rtc, &rtm[Y99]));

	/* Living after midnight */
	k_sleep(K_SECONDS(SECONDS_BEFORE + SECONDS_AFTER));
	zassert_ok(rtc_get_time(rtc, &rtm[Y2K]));

	/* It's the end of the world as we know it */
	zassert_equal(rtm[Y2K].tm_year + 1900, 2000, "wrong year: %d", rtm[Y2K].tm_year + 1900);
	zassert_equal(rtm[Y2K].tm_mon, 0, "wrong month: %d", rtm[Y2K].tm_mon);
	zassert_equal(rtm[Y2K].tm_mday, 1, "wrong day-of-month: %d", rtm[Y2K].tm_mday);
	zassert_true(rtm[Y2K].tm_yday == 0 || rtm[Y2K].tm_yday == -1, "wrong day-of-year: %d",
		     rtm[Y2K].tm_yday);
	zassert_true(rtm[Y2K].tm_wday == 6 || rtm[Y2K].tm_wday == -1, "wrong day-of-week: %d",
		     rtm[Y2K].tm_wday);
	zassert_equal(rtm[Y2K].tm_hour, 0, "wrong hour: %d", rtm[Y2K].tm_hour);
	zassert_equal(rtm[Y2K].tm_min, 0, "wrong minute: %d", rtm[Y2K].tm_min);
	zassert_equal(rtm[Y2K].tm_sec, SECONDS_AFTER, "wrong second: %d", rtm[Y2K].tm_sec);
}
