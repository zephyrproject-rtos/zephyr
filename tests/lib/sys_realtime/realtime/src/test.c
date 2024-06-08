/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/realtime.h>

#define TEST_START_TIMESTAMP_MS 946684795000 /* 1999-12-31T23:59:55.000Z */
#define TEST_DURATION_MS        10000
#define TEST_END_TIMESTAMP_MS   TEST_START_TIMESTAMP_MS + TEST_DURATION_MS
#define TEST_SLEEP_MS           100
#define TEST_THRESHOLD_MS       10

static const int64_t test_start_timestamp_ms = TEST_START_TIMESTAMP_MS;
static const int64_t test_end_timestamp_ms = TEST_END_TIMESTAMP_MS;

static int64_t test_sleep(void)
{
	int64_t uptime_before_ms;
	int64_t uptime_after_ms;

	uptime_before_ms = k_uptime_get();
	k_msleep(TEST_SLEEP_MS);
	uptime_after_ms = k_uptime_get();
	return uptime_after_ms - uptime_before_ms;
}

static void test_track_timestamp(void)
{
	int64_t timestamp_before_ms;
	int64_t timestamp_after_ms;
	int64_t timestamp_delta_ms;
	int64_t sleep_ms;
	int64_t lower_bound;
	int64_t upper_bound;

	do {
		zassert_ok(sys_realtime_get_timestamp(&timestamp_before_ms));
		sleep_ms = test_sleep();
		zassert_ok(sys_realtime_get_timestamp(&timestamp_after_ms));
		timestamp_delta_ms = timestamp_after_ms - timestamp_before_ms;
		lower_bound = sleep_ms - TEST_THRESHOLD_MS;
		upper_bound = sleep_ms + TEST_THRESHOLD_MS;
		zassert_true(timestamp_delta_ms >= lower_bound);
		zassert_true(timestamp_delta_ms <= upper_bound);
	} while (timestamp_after_ms < test_end_timestamp_ms);
}

static void test_track_datetime(void)
{
	struct sys_datetime datetime_before;
	struct sys_datetime datetime_after;
	int64_t timestamp_before_ms;
	int64_t timestamp_after_ms;
	int64_t timestamp_delta_ms;
	int64_t sleep_ms;
	int64_t lower_bound;
	int64_t upper_bound;

	do {
		zassert_ok(sys_realtime_get_datetime(&datetime_before));
		sleep_ms = test_sleep();
		zassert_ok(sys_realtime_get_datetime(&datetime_after));
		zassert_ok(sys_realtime_datetime_to_timestamp(&timestamp_before_ms,
							      &datetime_before));
		zassert_ok(sys_realtime_datetime_to_timestamp(&timestamp_after_ms,
							      &datetime_after));
		timestamp_delta_ms = timestamp_after_ms - timestamp_before_ms;
		lower_bound = sleep_ms - TEST_THRESHOLD_MS;
		upper_bound = sleep_ms + TEST_THRESHOLD_MS;
		zassert_true(timestamp_delta_ms >= lower_bound);
		zassert_true(timestamp_delta_ms <= upper_bound);
	} while (timestamp_after_ms < test_end_timestamp_ms);
}

ZTEST_USER(sys_realtime_realtime, test_set_and_track_timestamp)
{
	zassert_ok(sys_realtime_set_timestamp(&test_start_timestamp_ms));
	test_track_timestamp();
	zassert_ok(sys_realtime_set_timestamp(&test_start_timestamp_ms));
	test_track_timestamp();
}

ZTEST_USER(sys_realtime_realtime, test_set_and_track_datetime)
{
	struct sys_datetime datetime;

	sys_realtime_timestamp_to_datetime(&datetime, &test_start_timestamp_ms);
	zassert_ok(sys_realtime_set_datetime(&datetime));
	test_track_datetime();
	zassert_ok(sys_realtime_set_datetime(&datetime));
	test_track_datetime();
}

ZTEST_SUITE(sys_realtime_realtime, NULL, NULL, NULL, NULL, NULL);
