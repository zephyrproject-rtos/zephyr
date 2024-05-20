/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/realtime.h>
#include <string.h>

struct test_sample {
	struct sys_datetime datetime;
	int64_t timestamp_ms;
};

static const struct test_sample samples[] = {
	/* UTC timestamp = 0 */
	{
		.datetime = {
			.tm_sec = 0,
			.tm_min = 0,
			.tm_hour = 0,
			.tm_mday = 1,
			.tm_mon = 0,
			.tm_year = 70,
			.tm_wday = -1,
			.tm_yday = -1,
			.tm_isdst = -1,
			.tm_nsec = 0,
		},
		.timestamp_ms = 0,
	},
	/* (-0001)-12-31T23:59:59Z */
	{
		.datetime = {
			.tm_sec = 59,
			.tm_min = 59,
			.tm_hour = 23,
			.tm_mday = 31,
			.tm_mon = 11,
			.tm_year = -1901,
			.tm_wday = -1,
			.tm_yday = -1,
			.tm_isdst = -1,
			.tm_nsec = 0,
		},
		.timestamp_ms = -62167219201000,
	},
	/* 0000-01-01T00:00:00Z */
	{
		.datetime = {
			.tm_sec = 0,
			.tm_min = 0,
			.tm_hour = 0,
			.tm_mday = 1,
			.tm_mon = 0,
			.tm_year = -1900,
			.tm_wday = -1,
			.tm_yday = -1,
			.tm_isdst = -1,
			.tm_nsec = 0,
		},
		.timestamp_ms = -62167219200000,
	},
	/* 0000-01-01T00:00:01Z */
	{
		.datetime = {
			.tm_sec = 1,
			.tm_min = 0,
			.tm_hour = 0,
			.tm_mday = 1,
			.tm_mon = 0,
			.tm_year = -1900,
			.tm_wday = -1,
			.tm_yday = -1,
			.tm_isdst = -1,
			.tm_nsec = 0,
		},
		.timestamp_ms = -62167219199000,
	},
	/* 1999-12-31T23:59:59Z */
	{
		.datetime = {
			.tm_sec = 59,
			.tm_min = 59,
			.tm_hour = 23,
			.tm_mday = 31,
			.tm_mon = 11,
			.tm_year = 99,
			.tm_wday = -1,
			.tm_yday = -1,
			.tm_isdst = -1,
			.tm_nsec = 0,
		},
		.timestamp_ms = 946684799000,
	},
	/* 2000-01-01T00:00:00Z */
	{
		.datetime = {
			.tm_sec = 0,
			.tm_min = 0,
			.tm_hour = 0,
			.tm_mday = 1,
			.tm_mon = 0,
			.tm_year = 100,
			.tm_wday = -1,
			.tm_yday = -1,
			.tm_isdst = -1,
			.tm_nsec = 0,
		},
		.timestamp_ms = 946684800000,
	},
	/* 2000-01-01T00:00:01Z */
	{
		.datetime = {
			.tm_sec = 1,
			.tm_min = 0,
			.tm_hour = 0,
			.tm_mday = 1,
			.tm_mon = 0,
			.tm_year = 100,
			.tm_wday = -1,
			.tm_yday = -1,
			.tm_isdst = -1,
			.tm_nsec = 0,
		},
		.timestamp_ms = 946684801000,
	},
	/* 2399-12-31T23:59:59Z */
	{
		.datetime = {
			.tm_sec = 59,
			.tm_min = 59,
			.tm_hour = 23,
			.tm_mday = 31,
			.tm_mon = 11,
			.tm_year = 499,
			.tm_wday = -1,
			.tm_yday = -1,
			.tm_isdst = -1,
			.tm_nsec = 0,
		},
		.timestamp_ms = 13569465599000,
	},
	/* 2400-01-01T00:00:00Z */
	{
		.datetime = {
			.tm_sec = 0,
			.tm_min = 0,
			.tm_hour = 0,
			.tm_mday = 1,
			.tm_mon = 0,
			.tm_year = 500,
			.tm_wday = -1,
			.tm_yday = -1,
			.tm_isdst = -1,
			.tm_nsec = 0,
		},
		.timestamp_ms = 13569465600000,
	},
	/* 2400-01-01T00:00:01Z */
	{
		.datetime = {
			.tm_sec = 1,
			.tm_min = 0,
			.tm_hour = 0,
			.tm_mday = 1,
			.tm_mon = 0,
			.tm_year = 500,
			.tm_wday = -1,
			.tm_yday = -1,
			.tm_isdst = -1,
			.tm_nsec = 0,
		},
		.timestamp_ms = 13569465601000,
	},
	/* 2400-01-01T00:00:01.0001Z */
	{
		.datetime = {
			.tm_sec = 1,
			.tm_min = 0,
			.tm_hour = 0,
			.tm_mday = 1,
			.tm_mon = 0,
			.tm_year = 500,
			.tm_wday = -1,
			.tm_yday = -1,
			.tm_isdst = -1,
			.tm_nsec = 1000000,
		},
		.timestamp_ms = 13569465601001,
	},
	/* 2400-01-01T00:00:01.999Z */
	{
		.datetime = {
			.tm_sec = 1,
			.tm_min = 0,
			.tm_hour = 0,
			.tm_mday = 1,
			.tm_mon = 0,
			.tm_year = 500,
			.tm_wday = -1,
			.tm_yday = -1,
			.tm_isdst = -1,
			.tm_nsec = 999000000,
		},
		.timestamp_ms = 13569465601999,
	},
};

static bool datetimes_equal(const struct sys_datetime *a, const struct sys_datetime *b)
{
	return memcmp(a, b, sizeof(struct sys_datetime)) == 0;
}

ZTEST(sys_realtime_convert, test_timestamp_to_datetime)
{
	const struct sys_datetime *datetime;
	const int64_t *timestamp_ms;
	struct sys_datetime result;

	for (size_t i = 0; i < ARRAY_SIZE(samples); i++) {
		datetime = &samples[i].datetime;
		timestamp_ms = &samples[i].timestamp_ms;
		zassert_ok(sys_realtime_timestamp_to_datetime(&result, timestamp_ms),
			   "refused to convert %lli", *timestamp_ms);
		zassert_true(datetimes_equal(datetime, &result),
			     "incorrect conversion of %lli", *timestamp_ms);
	}
}

ZTEST(sys_realtime_convert, test_datetime_to_timestamp)
{
	const struct sys_datetime *datetime;
	const int64_t *timestamp_ms;
	int64_t result;

	for (size_t i = 0; i < ARRAY_SIZE(samples); i++) {
		datetime = &samples[i].datetime;
		timestamp_ms = &samples[i].timestamp_ms;
		zassert_ok(sys_realtime_datetime_to_timestamp(&result, datetime),
			   "refused to convert %lli", *timestamp_ms);
		zassert_equal(result, *timestamp_ms,
			     "incorrect conversion of %lli", *timestamp_ms);
	}
}

ZTEST_SUITE(sys_realtime_convert, NULL, NULL, NULL, NULL, NULL);
