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

#include <time.h>

/* Fri Jan 01 2021 13:29:50 GMT+0000 */
#define RTC_TEST_ALARM_SET_TIME		     (1609507790)
#define RTC_TEST_ALARM_TEST_NOT_CALLED_DELAY (3)
#define RTC_TEST_ALARM_TEST_CALLED_DELAY     (10)
#define RTC_TEST_ALARM_TIME_MINUTE	     (30)
#define RTC_TEST_ALARM_TIME_HOUR	     (13)

static const struct device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
static const uint16_t alarms_count = DT_PROP(DT_ALIAS(rtc), alarms_count);
static uint32_t callback_user_data_odd = 0x4321;
static uint32_t callback_user_data_even = 0x1234;
static atomic_t callback_called_mask_odd;
static atomic_t callback_called_mask_even;

static void test_rtc_alarm_callback_handler_odd(const struct device *dev, uint16_t id,
						void *user_data)
{
	atomic_set_bit(&callback_called_mask_odd, id);
}

static void test_rtc_alarm_callback_handler_even(const struct device *dev, uint16_t id,
						 void *user_data)
{
	atomic_set_bit(&callback_called_mask_even, id);
}

ZTEST(rtc_api, test_alarm_callback)
{
	int ret;
	time_t timer_set;
	struct rtc_time time_set;
	struct rtc_time alarm_time_set;
	uint16_t alarm_time_mask_supported;
	uint16_t alarm_time_mask_set;
	atomic_val_t callback_called_mask_status_odd;
	atomic_val_t callback_called_mask_status_even;
	bool callback_called_status;

	/* Disable alarm callback */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_set_callback(rtc, i, NULL, NULL);

		zassert_true(ret == 0, "Failed to clear and disable alarm");
	}

	/* Validate alarms supported fields */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_get_supported_fields(rtc, i, &alarm_time_mask_supported);

		zassert_true(ret == 0, "Failed to get supported alarm fields");

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

		zassert_true(ret == 0, "Failed to set alarm time");
	}

	/* Initialize RTC time to set */
	timer_set = RTC_TEST_ALARM_SET_TIME;

	gmtime_r(&timer_set, (struct tm *)(&time_set));

	time_set.tm_isdst = -1;
	time_set.tm_nsec = 0;

	/* Set RTC time */
	ret = rtc_set_time(rtc, &time_set);

	zassert_true(ret == 0, "Failed to set time");

	/* Clear alarm pending status */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_is_pending(rtc, i);

		zassert_true(ret > -1, "Failed to clear alarm pending status");
	}

	/* Set and enable alarm callback */
	for (uint16_t i = 0; i < alarms_count; i++) {
		if (i % 2) {
			ret = rtc_alarm_set_callback(rtc, i,
						test_rtc_alarm_callback_handler_odd,
						&callback_user_data_odd);
		} else {
			ret = rtc_alarm_set_callback(rtc, i,
						test_rtc_alarm_callback_handler_even,
						&callback_user_data_even);
		}

		zassert_true(ret == 0, "Failed to set alarm callback");
	}

	for (uint8_t i = 0; i < 2; i++) {
		/* Clear callback called atomics */
		atomic_set(&callback_called_mask_odd, 0);
		atomic_set(&callback_called_mask_even, 0);

		/* Wait before validating alarm callbacks have not been called prematurely */
		k_sleep(K_SECONDS(RTC_TEST_ALARM_TEST_NOT_CALLED_DELAY));

		/* Validate alarm callbacks have not been called prematurely */
		callback_called_mask_status_odd = atomic_get(&callback_called_mask_odd);
		callback_called_mask_status_even = atomic_get(&callback_called_mask_even);

		zassert_true(callback_called_mask_status_odd == 0,
			     "Alarm callback called prematurely");
		zassert_true(callback_called_mask_status_even == 0,
			     "Alarm callback called prematurely");

		/* Wait for alarm to trigger */
		k_sleep(K_SECONDS(RTC_TEST_ALARM_TEST_CALLED_DELAY));

		/* Validate alarm callback called */
		for (uint16_t i = 0; i < alarms_count; i++) {
			callback_called_status =
				(i % 2) ? atomic_test_bit(&callback_called_mask_odd, i)
					: atomic_test_bit(&callback_called_mask_even, i);

			zassert_true(callback_called_status == true,
				     "Alarm callback should have been called");
		}

		/* Reset RTC time */
		ret = rtc_set_time(rtc, &time_set);

		zassert_true(ret == 0, "Failed to set time");
	}

	/* Disable and clear alarms */
	for (uint16_t i = 0; i < alarms_count; i++) {
		ret = rtc_alarm_set_callback(rtc, i, NULL, NULL);

		zassert_true(ret == 0, "Failed to disable alarm callback");

		ret = rtc_alarm_set_time(rtc, i, 0, NULL);

		zassert_true(ret == 0, "Failed to disable alarm");

		ret = rtc_alarm_is_pending(rtc, i);

		zassert_true(ret > -1, "Failed to clear alarm pending state");
	}
}
