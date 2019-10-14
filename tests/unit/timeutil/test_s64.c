/*
 * Copyright (c) 2019 Peter Bigot Consulting
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Tests where time_t requires a 64-bit value */

#include <errno.h>
#include <ztest.h>
#include "timeutil_test.h"

static const struct timeutil_test_data tests[] = {
	/* 32-bit, but algorithm subtraction underflows */
	{ .ux = -2147483648,
	  .civil = "1901-12-13 20:45:52 Fri 347",
	  .tm = {
		  .tm_sec = 52,
		  .tm_min = 45,
		  .tm_hour = 20,
		  .tm_mday = 13,
		  .tm_mon = 11,
		  .tm_year = 1,
		  .tm_wday = 5,
		  .tm_yday = 346,
	  } },
	{ .ux = (time_t)-2147483649,
	  .civil = "1901-12-13 20:45:51 Fri 347",
	  .tm = {
		  .tm_sec = 51,
		  .tm_min = 45,
		  .tm_hour = 20,
		  .tm_mday = 13,
		  .tm_mon = 11,
		  .tm_year = 1,
		  .tm_wday = 5,
		  .tm_yday = 346,
	  } },
	{ .ux = (time_t)2147483648,
	  .civil = "2038-01-19 03:14:08 Tue 019",
	  .tm = {
		  .tm_sec = 8,
		  .tm_min = 14,
		  .tm_hour = 3,
		  .tm_mday = 19,
		  .tm_mon = 0,
		  .tm_year = 138,
		  .tm_wday = 2,
		  .tm_yday = 18,
	  } },
	{ .ux = (time_t)64060588799,
	  .civil = "3999-12-31 23:59:59 Fri 365",
	  .tm = {
		  .tm_sec = 59,
		  .tm_min = 59,
		  .tm_hour = 23,
		  .tm_mday = 31,
		  .tm_mon = 11,
		  .tm_year = 2099,
		  .tm_wday = 5,
		  .tm_yday = 364,
	  } },
	{ .ux = (time_t)64060588800,
	  .civil = "4000-01-01 00:00:00 Sat 001",
	  .tm = {
		  .tm_sec = 0,
		  .tm_min = 0,
		  .tm_hour = 0,
		  .tm_mday = 1,
		  .tm_mon = 0,
		  .tm_year = 2100,
		  .tm_wday = 6,
		  .tm_yday = 0,
	  } },
	/* Normal century is a common year */
	{ .ux = (time_t)-2208988801,
	  .civil = "1899-12-31 23:59:59 Sun 365",
	  .tm = {
		  .tm_sec = 59,
		  .tm_min = 59,
		  .tm_hour = 23,
		  .tm_mday = 31,
		  .tm_mon = 11,
		  .tm_year = -1,
		  .tm_wday = 0,
		  .tm_yday = 364,
	  } },
	{ .ux = (time_t)-2208988800,
	  .civil = "1900-01-01 00:00:00 Mon 001",
	  .tm = {
		  .tm_sec = 0,
		  .tm_min = 0,
		  .tm_hour = 0,
		  .tm_mday = 1,
		  .tm_mon = 0,
		  .tm_year = 0,
		  .tm_wday = 1,
		  .tm_yday = 0,
	  } },
	{ .ux = (time_t)-2203977600,
	  .civil = "1900-02-28 00:00:00 Wed 059",
	  .tm = {
		  .tm_sec = 0,
		  .tm_min = 0,
		  .tm_hour = 0,
		  .tm_mday = 28,
		  .tm_mon = 1,
		  .tm_year = 0,
		  .tm_wday = 3,
		  .tm_yday = 58,
	  } },
	{ .ux = (time_t)-2203891200,
	  .civil = "1900-03-01 00:00:00 Thu 060",
	  .tm = {
		  .tm_sec = 0,
		  .tm_min = 0,
		  .tm_hour = 0,
		  .tm_mday = 1,
		  .tm_mon = 2,
		  .tm_year = 0,
		  .tm_wday = 4,
		  .tm_yday = 59,
	  } },
	{ .ux = (time_t)-2177539200,
	  .civil = "1900-12-31 00:00:00 Mon 365",
	  .tm = {
		  .tm_sec = 0,
		  .tm_min = 0,
		  .tm_hour = 0,
		  .tm_mday = 31,
		  .tm_mon = 11,
		  .tm_year = 0,
		  .tm_wday = 1,
		  .tm_yday = 364,
	  } },
	{ .ux = (time_t)-2177452800,
	  .civil = "1901-01-01 00:00:00 Tue 001",
	  .tm = {
		  .tm_sec = 0,
		  .tm_min = 0,
		  .tm_hour = 0,
		  .tm_mday = 1,
		  .tm_mon = 0,
		  .tm_year = 1,
		  .tm_wday = 2,
		  .tm_yday = 0,
	  } },

	/* Extrema, check against proleptic Gregorian calendar data:
	 * https://www.timeanddate.com/calendar/?year=1&country=22
	 */
	{ .ux = (time_t)-62167305600,
	  .civil = "-1-12-31 00:00:00 Fri 365",
	  .tm = {
		  .tm_sec = 0,
		  .tm_min = 0,
		  .tm_hour = 0,
		  .tm_mday = 31,
		  .tm_mon = 11,
		  .tm_year = -1901,
		  .tm_wday = 5,
		  .tm_yday = 364,
	  } },
	{ .ux = (time_t)-62167219200,
	  .civil = "0-01-01 00:00:00 Sat 001",
	  .tm = {
		  .tm_sec = 0,
		  .tm_min = 0,
		  .tm_hour = 0,
		  .tm_mday = 1,
		  .tm_mon = 0,
		  .tm_year = -1900,
		  .tm_wday = 6,
		  .tm_yday = 0,
	  } },
	{ .ux = (time_t)-62135596801,
	  .civil = "0-12-31 23:59:59 Sun 366",
	  .tm = {
		  .tm_sec = 59,
		  .tm_min = 59,
		  .tm_hour = 23,
		  .tm_mday = 31,
		  .tm_mon = 11,
		  .tm_year = -1900,
		  .tm_wday = 0,
		  .tm_yday = 365,
	  } },
	{ .ux = (time_t)-62135596800,
	  .civil = "1-01-01 00:00:00 Mon 001",
	  .tm = {
		  .tm_sec = 0,
		  .tm_min = 0,
		  .tm_hour = 0,
		  .tm_mday = 1,
		  .tm_mon = 0,
		  .tm_year = -1899,
		  .tm_wday = 1,
		  .tm_yday = 0,
	  } },
	{ .ux = (time_t)-62135596800,
	  .civil = "1-01-01 00:00:00 Mon 001",
	  .tm = {
		  .tm_sec = 0,
		  .tm_min = 0,
		  .tm_hour = 0,
		  .tm_mday = 1,
		  .tm_mon = 0,
		  .tm_year = -1899,
		  .tm_wday = 1,
		  .tm_yday = 0,
	  } },
};

static void test_time32_errno_clear(void)
{
	const struct timeutil_test_data *tp = &(const struct timeutil_test_data){
		.ux = 0,
		.civil = "1970-01-01 00:00:00 Thu 001",
		.tm = {
			.tm_sec = 0,
			.tm_min = 0,
			.tm_hour = 0,
			.tm_mday = 1,
			.tm_mon = 0,
			.tm_year = 70,
			.tm_wday = 4,
			.tm_yday = 0,
		},
	};

	errno = EINVAL;

	time_t ux = timeutil_timegm(&tp->tm);

	zassert_equal(ux, tp->ux,
		      "conversion incorrect");
	zassert_equal(errno, 0,
		      "errno was not cleared");
}

static void test_time32_epochm1(void)
{
	const struct timeutil_test_data *tp = &(const struct timeutil_test_data){
		.ux = -1,
		.civil = "1969-12-31 23:59:59 Wed 365",
		.tm = {
			.tm_sec = 59,
			.tm_min = 59,
			.tm_hour = 23,
			.tm_mday = 31,
			.tm_mon = 11,
			.tm_year = 69,
			.tm_wday = 3,
			.tm_yday = 364,
		},
	};

	errno = EINVAL;

	time_t ux = timeutil_timegm(&tp->tm);

	zassert_equal(ux, tp->ux,
		      "conversion incorrect");
	zassert_equal(errno, 0,
		      "final errno state bad");
}

static void test_time32_underflow(void)
{
	const s64_t unix64 = -2147483649;
	const struct timeutil_test_data *tp = &(const struct timeutil_test_data){
		.civil = "1901-12-13 20:45:51 Fri 347",
		.tm = {
			.tm_sec = 51,
			.tm_min = 45,
			.tm_hour = 20,
			.tm_mday = 13,
			.tm_mon = 11,
			.tm_year = 1,
			.tm_wday = 5,
			.tm_yday = 346,
		},
	};

	zassert_equal(timeutil_timegm64(&tp->tm), unix64,
		      "fullscale failed");
	errno = 0;

	time_t ux = timeutil_timegm(&tp->tm);

	zassert_equal(ux, -1,
		      "underflow undetected");
	zassert_equal(errno, ERANGE,
		      "final errno state bad");
}

static void test_time32_overflow(void)
{
	const s64_t unix64 = 2147483648;
	const struct timeutil_test_data *tp = &(const struct timeutil_test_data){
		.civil = "2038-01-19 03:14:08 Tue 019",
		.tm = {
			.tm_sec = 8,
			.tm_min = 14,
			.tm_hour = 3,
			.tm_mday = 19,
			.tm_mon = 0,
			.tm_year = 138,
			.tm_wday = 2,
			.tm_yday = 18,
		},
	};

	zassert_equal(timeutil_timegm64(&tp->tm), unix64,
		      "fullscale failed");
	errno = 0;

	time_t ux = timeutil_timegm(&tp->tm);

	zassert_equal(ux, -1,
		      "overflow undetected");
	zassert_equal(errno, ERANGE,
		      "final errno state bad");
}

void test_s64(void)
{
	if (sizeof(time_t) < 8U) {
		test_time32_errno_clear();
		test_time32_epochm1();
		test_time32_underflow();
		test_time32_overflow();
		return;
	}
	timeutil_check(tests, sizeof(tests) / sizeof(*tests));
}
