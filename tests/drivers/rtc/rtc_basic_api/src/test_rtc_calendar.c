/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_rtc_basic_api
 * @{
 * @defgroup t_rtc_calendar test_rtc_calendar
 * @brief TestPurpose: verify RTC internal counter enable/disable/read work
 * @details
 * - Test Steps
 *   -# Enable RTC internal counter
 *   -# Read RTC internal counter and sleep for a while
 *   -# Read RTC internal counter again and compare it with the previous value
 *   -# Disable RTC internal counter and repeat previous operations
 * - Expected Results
 *   -# When enabled, the values read from RTC internal counter increase.
 *   -# When disabled, the values read from RTC internal counter don't change.
 * @}
 */

#include "test_rtc.h"

static int test_task(void)
{
	u32_t val_1 = 0;
	u32_t val_2 = 0;
	struct device *rtc = device_get_binding(RTC_DEVICE_NAME);

	if (!rtc) {
		TC_PRINT("Cannot get RTC device\n");
		return TC_FAIL;
	}

	/* 1. Verify rtc_enable() */
	rtc_enable(rtc);

	/* 2. Verify rtc_enable() */
	val_1 = rtc_read(rtc);
	k_sleep(2000);
	val_2 = rtc_read(rtc);

	TC_PRINT("val_1: %u, val_2: %u, delta: %lu:%lu\n",
		 val_1, val_2,
		 (val_2 - val_1) / RTC_ALARM_SECOND,
		 (val_2 - val_1) % RTC_ALARM_SECOND);

	if (val_2 <= val_1) {
		TC_PRINT("RTC doesn't work well\n");
		return TC_FAIL;
	}

	/* 3. Verify rtc_disable() */
	rtc_disable(rtc);

	val_1 = rtc_read(rtc);
	k_sleep(1000);
	val_2 = rtc_read(rtc);

	if (val_2 != val_1) {
		TC_PRINT("Fail to disable RTC\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

void test_rtc_calendar(void)
{
	zassert_true((test_task() == TC_PASS), NULL);
}
