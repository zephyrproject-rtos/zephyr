/*
 * Copyright 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/rtc.h>

struct test_sample {
	uint32_t frequency;
	int32_t calibration;
};

static const struct test_sample test_samples[] = {
	{
		.frequency = 1000000000,
		.calibration = 0,
	},
	{
		.frequency = 1000000001,
		.calibration = -1,
	},
	{
		.frequency = 999999999,
		.calibration = 1,
	},
	{
		.frequency = 2000000000,
		.calibration = -500000000,
	},
	{
		.frequency = 500000000,
		.calibration = 1000000000,
	},
};

ZTEST(rtc_api_helpers, test_validate_calibration_from_frequency)
{
	uint32_t frequency;
	int32_t calibration;
	int32_t result;

	ARRAY_FOR_EACH(test_samples, i) {
		frequency = test_samples[i].frequency;
		calibration = test_samples[i].calibration;
		result = rtc_calibration_from_frequency(frequency);
		zassert_equal(result, calibration);
	}
}
