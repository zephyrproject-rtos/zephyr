/*
 * Copyright (c) 2022 Bjarki Arge Andreasen
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#define RTC_TEST_ALARM_TEST_NOT_PENDING_DELAY (3)
#define RTC_TEST_ALARM_TEST_PENDING_DELAY     (10)

static const struct device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
static const uint16_t alarms_count = DT_PROP(DT_ALIAS(rtc), alarms_count);
static const uint16_t test_alarm_time_mask_set = CONFIG_TEST_RTC_ALARM_TIME_MASK;

/* Fri Jan 01 2021 13:29:50 GMT+0000 */
static const struct rtc_time test_rtc_time_set = {
	.tm_sec = 50,
	.tm_min = 29,
	.tm_hour = 13,
	.tm_mday = 1,
	.tm_mon = 0,
	.tm_year = 121,
	.tm_wday = 5,
	.tm_yday = 1,
	.tm_isdst = -1,
	.tm_nsec = 0,
};

/* Fri Jan 01 2021 13:30:00 GMT+0000 */
static const struct rtc_time test_alarm_time_set = {
	.tm_sec = 0,
	.tm_min = 30,
	.tm_hour = 13,
	.tm_mday = 1,
	.tm_mon = 0,
	.tm_year = 121,
	.tm_wday = 5,
	.tm_yday = 1,
	.tm_isdst = -1,
	.tm_nsec = 0,
};

static const struct rtc_time test_alarm_time_invalid = {
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

static const uint16_t test_alarm_time_masks[] = {
	RTC_ALARM_TIME_MASK_SECOND,  RTC_ALARM_TIME_MASK_MINUTE,
	RTC_ALARM_TIME_MASK_HOUR,    RTC_ALARM_TIME_MASK_MONTHDAY,
	RTC_ALARM_TIME_MASK_MONTH,   RTC_ALARM_TIME_MASK_YEAR,
	RTC_ALARM_TIME_MASK_WEEKDAY, RTC_ALARM_TIME_MASK_YEARDAY,
	RTC_ALARM_TIME_MASK_NSEC
};

ZTEST(rtc_api, test_alarm)
{
	int ret;
	uint16_t alarm_time_mask_supported;
	struct rtc_time alarm_time_get;
	uint16_t alarm_time_mask_get;

	/* Clear alarm alarm time */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_set_time(rtc, i, 0, NULL);

		zassert_ok(ret, "Failed to clear alarm %d time", i);
	}

	/* Disable alarm callback */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_set_callback(rtc, i, NULL, NULL);

		zassert_true((ret == 0) || (ret == -ENOTSUP),
			     "Failed to clear and disable alarm %d callback", i);
	}

	/* Every supported alarm field should reject invalid values. */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_get_supported_fields(rtc, i, &alarm_time_mask_supported);
		zassert_ok(ret, "Failed to get supported alarm %d fields", i);

		ARRAY_FOR_EACH(test_alarm_time_masks, j)
		{
			if (test_alarm_time_masks[j] & alarm_time_mask_supported) {
				ret = rtc_alarm_set_time(rtc, i, test_alarm_time_masks[j],
							 &test_alarm_time_invalid);
				zassert_equal(
					-EINVAL, ret,
					"%s: RTC should reject invalid alarm %d time in field %zu.",
					rtc->name, i, j);
			}
		}
	}

	/* Validate alarms supported fields */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_get_supported_fields(rtc, i, &alarm_time_mask_supported);
		zassert_ok(ret, "Failed to get supported alarm %d fields", i);

		ret = (test_alarm_time_mask_set & (~alarm_time_mask_supported)) ? -EINVAL : 0;
		zassert_ok(ret, "Configured alarm time fields to set are not supported");
	}

	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_set_time(rtc, i, test_alarm_time_mask_set, &test_alarm_time_set);
		zassert_ok(ret, "Failed to set alarm %d time", i);
	}

	/* Validate alarm time */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_get_time(rtc, i, &alarm_time_mask_get, &alarm_time_get);
		zassert_ok(ret, "Failed to set alarm %d time", i);

		zassert_equal(alarm_time_mask_get, test_alarm_time_mask_set,
			      "Incorrect alarm %d time mask", i);

		if (test_alarm_time_mask_set & RTC_ALARM_TIME_MASK_SECOND) {
			zassert_equal(alarm_time_get.tm_sec, test_alarm_time_set.tm_sec,
				      "Incorrect alarm %d tm_sec field", i);
		}

		if (test_alarm_time_mask_set & RTC_ALARM_TIME_MASK_MINUTE) {
			zassert_equal(alarm_time_get.tm_min, test_alarm_time_set.tm_min,
				      "Incorrect alarm %d tm_min field", i);
		}

		if (test_alarm_time_mask_set & RTC_ALARM_TIME_MASK_HOUR) {
			zassert_equal(alarm_time_get.tm_hour, test_alarm_time_set.tm_hour,
				      "Incorrect alarm %d tm_hour field", i);
		}

		if (test_alarm_time_mask_set & RTC_ALARM_TIME_MASK_MONTHDAY) {
			zassert_equal(alarm_time_get.tm_mday, test_alarm_time_set.tm_mday,
				      "Incorrect alarm %d tm_mday field", i);
		}

		if (test_alarm_time_mask_set & RTC_ALARM_TIME_MASK_MONTH) {
			zassert_equal(alarm_time_get.tm_mon, test_alarm_time_set.tm_mon,
				      "Incorrect alarm %d tm_mon field", i);
		}

		if (test_alarm_time_mask_set & RTC_ALARM_TIME_MASK_YEAR) {
			zassert_equal(alarm_time_get.tm_year, test_alarm_time_set.tm_year,
				      "Incorrect alarm %d tm_year field", i);
		}

		if (test_alarm_time_mask_set & RTC_ALARM_TIME_MASK_WEEKDAY) {
			zassert_equal(alarm_time_get.tm_wday, test_alarm_time_set.tm_wday,
				      "Incorrect alarm %d tm_wday field", i);
		}

		if (test_alarm_time_mask_set & RTC_ALARM_TIME_MASK_YEARDAY) {
			zassert_equal(alarm_time_get.tm_yday, test_alarm_time_set.tm_yday,
				      "Incorrect alarm %d tm_yday field", i);
		}

		if (test_alarm_time_mask_set & RTC_ALARM_TIME_MASK_NSEC) {
			zassert_equal(alarm_time_get.tm_nsec, test_alarm_time_set.tm_nsec,
				      "Incorrect alarm %d tm_nsec field", i);
		}
	}

	for (uint8_t k = 0; k < 2; k++) {
		/* Set RTC time */
		ret = rtc_set_time(rtc, &test_rtc_time_set);
		zassert_ok(ret, "Failed to set time");

		/* Clear alarm pending status */
		for (uint16_t i = 0; i < alarms_count; i++) {
			ret = rtc_alarm_is_pending(rtc, i);
			zassert_true(ret > -1, "Failed to clear alarm %d pending status", i);
		}

		/* Wait before validating alarm pending status has not been set prematurely */
		k_sleep(K_SECONDS(RTC_TEST_ALARM_TEST_NOT_PENDING_DELAY));

		/* Validate alarm are not pending */
		for (uint16_t i = 0; i < alarms_count; i++) {
			ret = rtc_alarm_is_pending(rtc, i);
			zassert_ok(ret, "Alarm %d should not be pending", i);
		}

		/* Wait for alarm to trigger */
		k_sleep(K_SECONDS(RTC_TEST_ALARM_TEST_PENDING_DELAY));

		/* Validate alarm is pending */
		for (uint16_t i = 0; i < alarms_count; i++) {
			ret = rtc_alarm_is_pending(rtc, i);
			zassert_equal(ret, 1, "Alarm %d should be pending", i);
		}
	}

	/* Disable and clear alarms */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_set_time(rtc, i, 0, NULL);
		zassert_ok(ret, "Failed to disable alarm %d", i);

		ret = rtc_alarm_is_pending(rtc, i);
		zassert_true(ret > -1, "Failed to clear alarm %d pending state", i);
	}
}
