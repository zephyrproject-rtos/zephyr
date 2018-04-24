/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_rtc_basic_api
 * @{
 * @defgroup t_rtc_alarm test_rtc_alarm
 * @brief TestPurpose: verify RTC alarm work and pending interrupt detected
 * @details
 * - Test Steps
 *   -# Enable RTC internal counter
 *   -# Configure RTC with init_val, alarm_val, cb_fn and enable RTC alarm
 *   -# Sleep for while waiting for RTC alarm
 *   -# Reconfigure RTC to alarm 1 second later using rtc_set_alarm()
 *   -# Sleep for while waiting for RTC alarm
 *   -# Disable RTC internal counter and repeat previous operations
 * - Expected Results
 *   -# When RTC internal counter is enabled, RTC alarm can be invoked using
 *      both rtc_set_config() and rtc_set_alarm()
 *   -# When RTC internal counter is disabled, RTC alarm won't be invoked.
 * @}
 */

#include "test_rtc.h"

static bool rtc_alarm_up;

static void rtc_alarm_callback(struct device *rtc_dev)
{

	TC_PRINT("%s: Invoked\n", __func__);
	TC_PRINT("RTC counter: %u\n", rtc_read(rtc_dev));

	/* Verify rtc_get_pending_int() */
	if (rtc_get_pending_int(rtc_dev)) {
		TC_PRINT("Catch pending RTC interrupt\n");
	} else {
		TC_PRINT("Fail to catch pending RTC interrupt\n");
	}

	rtc_alarm_up = true;
}

static int test_alarm(void)
{
	struct rtc_config config;
	struct device *rtc = device_get_binding(RTC_DEVICE_NAME);

	if (!rtc) {
		TC_PRINT("Cannot get RTC device\n");
		return TC_FAIL;
	}

	config.init_val = 0;
	config.alarm_enable = 1;
	config.alarm_val = RTC_ALARM_SECOND;
	config.cb_fn = rtc_alarm_callback;

	rtc_enable(rtc);

	/* 1. Verify rtc_set_config() */
	rtc_alarm_up = false;
	if (rtc_set_config(rtc, &config)) {
		TC_ERROR("Failed to config RTC alarm\n");
		return TC_FAIL;
	}

	k_sleep(2000);

	if (!rtc_alarm_up) {
		TC_PRINT("RTC alarm doesn't work well\n");
		return TC_FAIL;
	}

	/* 2. Verify rtc_set_alarm() */
	rtc_alarm_up = false;
	if (rtc_set_alarm(rtc, rtc_read(rtc) + RTC_ALARM_SECOND)) {
		TC_PRINT("Failed to set RTC Alarm\n");
		return TC_FAIL;
	}

	k_sleep(2000);

	if (!rtc_alarm_up) {
		TC_PRINT("RTC alarm doesn't work well\n");
		return TC_FAIL;
	}

	/* 3. Verify RTC Alarm disabled after disable internal counter */
	rtc_disable(rtc);
	rtc_alarm_up = false;

	if (rtc_set_alarm(rtc, rtc_read(rtc) + RTC_ALARM_SECOND)) {
		TC_PRINT("Failed to set RTC Alarm\n");
		return TC_FAIL;
	}

	k_sleep(1500);

	if (rtc_alarm_up) {
		TC_PRINT("Failed to disable RTC Alarm\n");
		return TC_FAIL;
	}

	TC_PRINT("RTC alarm works well\n");

	return TC_PASS;
}

void test_rtc_alarm(void)
{
	zassert_true(test_alarm() == TC_PASS, NULL);
}
