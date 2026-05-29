/*
 * Copyright 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>

#define RTC_TEST_CAL_RANGE_LIMIT (200000)
#define RTC_TEST_CAL_RANGE_STEP	 (10000)

static const struct device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

static int test_set_get_calibration(int32_t calibrate_set)
{
	int32_t calibrate_get;
	int ret;

	ret = rtc_set_calibration(rtc, calibrate_set);

	/* Check if calibration is within limits of hardware */
	if (ret == -EINVAL) {
		/* skip */
		return -EINVAL;
	}

	/* Validate calibration was set */
	zassert_ok(ret, "Failed to set calibration");

	ret = rtc_get_calibration(rtc, &calibrate_get);

	/* Validate calibration was gotten */
	zassert_ok(ret, "Failed to get calibration");

	/* Print comparison between set and get values */
	printk("Calibrate (set,get): %i, %i\n", calibrate_set, calibrate_get);

	return 0;
}

ZTEST(rtc_api, test_set_get_calibration)
{
	int32_t calibrate_get;
	int ret;
	int32_t range_limit;
	int32_t range_step;

	ret = rtc_set_calibration(rtc, 0);

	/* Validate calibration was set */
	zassert_ok(ret, "Failed to set calibration");

	ret = rtc_get_calibration(rtc, &calibrate_get);

	/* Validate calibration was gotten */
	zassert_ok(ret, "Failed to get calibration");

	/* Validate edge values (0 already tested) */
	test_set_get_calibration(1);
	test_set_get_calibration(-1);

	range_limit = RTC_TEST_CAL_RANGE_LIMIT;
	range_step = RTC_TEST_CAL_RANGE_STEP;

	/* Validate over negative range */
	for (int32_t set = range_step; set <= range_limit; set += range_step) {
		ret = test_set_get_calibration(-set);

		if (ret < 0) {
			/* Limit of hardware capabilties reached */
			break;
		}
	}

	/* Validate over positive range */
	for (int32_t set = range_step; set <= range_limit; set += range_step) {
		ret = test_set_get_calibration(set);

		if (ret < 0) {
			/* Limit of hardware capabilties reached */
			break;
		}
	}
}
