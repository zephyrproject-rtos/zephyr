/*
 * Copyright 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>

#include <time.h>

/* Fri Jan 01 2021 13:29:50 GMT+0000 */
#define RTC_TEST_ALARM_SET_TIME		      (1609507790)
#define RTC_TEST_ALARM_TEST_NOT_PENDING_DELAY (3)
#define RTC_TEST_ALARM_TEST_PENDING_DELAY     (10)
#define RTC_TEST_ALARM_TIME_MINUTE	      (30)
#define RTC_TEST_ALARM_TIME_HOUR	      (13)

static const struct device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
static const uint16_t alarms_count = DT_PROP(DT_ALIAS(rtc), alarms_count);

ZTEST(rtc_api, test_alarm)
{
	int ret;
	time_t timer_set;
	struct rtc_time time_set;
	struct rtc_time alarm_time_set;
	uint16_t alarm_time_mask_supported;
	uint16_t alarm_time_mask_set;
	struct rtc_time alarm_time_get;
	uint16_t alarm_time_mask_get;

	/* Clear alarm alarm time */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_set_time(rtc, i, 0, NULL);

		zassert_ok(ret, "Failed to clear alarm time");
	}

	/* Disable alarm callback */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_set_callback(rtc, i, NULL, NULL);

		zassert_true((ret == 0) || (ret == -ENOTSUP),
			     "Failed to clear and disable alarm callback");
	}

	/* Every supported alarm field should reject invalid values. */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_get_supported_fields(rtc, i, &alarm_time_mask_supported);

		zassert_ok(ret, "Failed to get supported alarm fields");

		alarm_time_set = (struct rtc_time) {
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
			if (masks[j] & alarm_time_mask_supported) {
				ret = rtc_alarm_set_time(rtc, i, masks[j], &alarm_time_set);
				zassert_equal(
					-EINVAL, ret,
					"%s: RTC should reject invalid alarm time in field %zu.",
					rtc->name, j);
			}
		}
	}

	/* Validate alarms supported fields */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_get_supported_fields(rtc, i, &alarm_time_mask_supported);

		zassert_ok(ret, "Failed to get supported alarm fields");

		/* Skip test if alarm does not support the minute and hour fields */
		if (((RTC_ALARM_TIME_MASK_MINUTE & alarm_time_mask_supported) == 0) ||
		((RTC_TEST_ALARM_TIME_HOUR & alarm_time_mask_supported) == 0)) {
			ztest_test_skip();
		}
	}

	/* Set alarm time */
	alarm_time_set.tm_min = RTC_TEST_ALARM_TIME_MINUTE;
	alarm_time_set.tm_hour = RTC_TEST_ALARM_TIME_HOUR;
	alarm_time_mask_set = (RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR);

	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_set_time(rtc, i, alarm_time_mask_set, &alarm_time_set);

		zassert_ok(ret, "Failed to set alarm time");
	}

	/* Validate alarm time */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_get_time(rtc, i, &alarm_time_mask_get, &alarm_time_get);

		zassert_ok(ret, "Failed to set alarm time");

		zassert_equal(alarm_time_mask_get, alarm_time_mask_set,
			      "Incorrect alarm time mask");

		zassert_equal(alarm_time_get.tm_min, alarm_time_get.tm_min,
			      "Incorrect alarm time minute field");

		zassert_equal(alarm_time_get.tm_hour, alarm_time_get.tm_hour,
			      "Incorrect alarm time hour field");
	}

	/* Initialize RTC time to set */
	timer_set = RTC_TEST_ALARM_SET_TIME;

	gmtime_r(&timer_set, (struct tm *)(&time_set));

	time_set.tm_isdst = -1;
	time_set.tm_nsec = 0;

	for (uint8_t k = 0; k < 2; k++) {
		/* Set RTC time */
		ret = rtc_set_time(rtc, &time_set);

		zassert_ok(ret, "Failed to set time");

		/* Clear alarm pending status */
		for (uint16_t i = 0; i < alarms_count; i++) {
			ret = rtc_alarm_is_pending(rtc, i);

			zassert_true(ret > -1, "Failed to clear alarm pending status");
		}

		/* Wait before validating alarm pending status has not been set prematurely */
		k_sleep(K_SECONDS(RTC_TEST_ALARM_TEST_NOT_PENDING_DELAY));

		/* Validate alarm are not pending */
		for (uint16_t i = 0; i < alarms_count; i++) {
			ret = rtc_alarm_is_pending(rtc, i);

			zassert_ok(ret, "Alarm should not be pending");
		}

		/* Wait for alarm to trigger */
		k_sleep(K_SECONDS(RTC_TEST_ALARM_TEST_PENDING_DELAY));

		/* Validate alarm is pending */
		for (uint16_t i = 0; i < alarms_count; i++) {
			ret = rtc_alarm_is_pending(rtc, i);

			zassert_equal(ret, 1, "Alarm should be pending");
		}
	}

	/* Disable and clear alarms */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_set_time(rtc, i, 0, NULL);

		zassert_ok(ret, "Failed to disable alarm");

		ret = rtc_alarm_is_pending(rtc, i);

		zassert_true(ret > -1, "Failed to clear alarm pending state");
	}
}
