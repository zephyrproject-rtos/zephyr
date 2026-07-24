/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>

struct gain_desc {
	uint8_t mul;
	uint8_t div;
};
static const struct gain_desc gains[] = {
	[ADC_GAIN_1_6] = {.mul = 1, .div = 6},
	[ADC_GAIN_1_5] = {.mul = 1, .div = 5},
	[ADC_GAIN_1_4] = {.mul = 1, .div = 4},
	[ADC_GAIN_2_7] = {.mul = 2, .div = 7},
	[ADC_GAIN_1_3] = {.mul = 1, .div = 3},
	[ADC_GAIN_2_5] = {.mul = 2, .div = 5},
	[ADC_GAIN_1_2] = {.mul = 1, .div = 2},
	[ADC_GAIN_2_3] = {.mul = 2, .div = 3},
	[ADC_GAIN_4_5] = {.mul = 4, .div = 5},
	[ADC_GAIN_1] = {.mul = 1, .div = 1},
	[ADC_GAIN_2] = {.mul = 2, .div = 1},
	[ADC_GAIN_3] = {.mul = 3, .div = 1},
	[ADC_GAIN_4] = {.mul = 4, .div = 1},
	[ADC_GAIN_6] = {.mul = 6, .div = 1},
	[ADC_GAIN_8] = {.mul = 8, .div = 1},
	[ADC_GAIN_12] = {.mul = 12, .div = 1},
	[ADC_GAIN_16] = {.mul = 16, .div = 1},
	[ADC_GAIN_24] = {.mul = 24, .div = 1},
	[ADC_GAIN_32] = {.mul = 32, .div = 1},
	[ADC_GAIN_64] = {.mul = 64, .div = 1},
	[ADC_GAIN_128] = {.mul = 128, .div = 1},
};

int adc_gain_apply(enum adc_gain gain, int32_t *value)
{
	int rv = -EINVAL;

	if ((uint8_t)gain < ARRAY_SIZE(gains)) {
		const struct gain_desc *gdp = &gains[gain];

		__ASSERT_NO_MSG(gdp->mul != 0);
		__ASSERT_NO_MSG(gdp->div != 0);
		*value = (gdp->mul * *value) / gdp->div;
		rv = 0;
	}

	return rv;
}

int adc_gain_apply_64(enum adc_gain gain, int64_t *value)
{
	int rv = -EINVAL;

	if ((uint8_t)gain < ARRAY_SIZE(gains)) {
		const struct gain_desc *gdp = &gains[gain];

		__ASSERT_NO_MSG(gdp->mul != 0);
		__ASSERT_NO_MSG(gdp->div != 0);
		*value = (gdp->mul * *value) / gdp->div;
		rv = 0;
	}

	return rv;
}

int adc_gain_invert(enum adc_gain gain, int32_t *value)
{
	int rv = -EINVAL;

	if ((uint8_t)gain < ARRAY_SIZE(gains)) {
		const struct gain_desc *gdp = &gains[gain];

		__ASSERT_NO_MSG(gdp->mul != 0);
		__ASSERT_NO_MSG(gdp->div != 0);
		*value = (gdp->div * *value) / gdp->mul;
		rv = 0;
	}

	return rv;
}

int adc_gain_invert_64(enum adc_gain gain, int64_t *value)
{
	int rv = -EINVAL;

	if ((uint8_t)gain < ARRAY_SIZE(gains)) {
		const struct gain_desc *gdp = &gains[gain];

		__ASSERT_NO_MSG(gdp->mul != 0);
		__ASSERT_NO_MSG(gdp->div != 0);
		*value = (gdp->div * *value) / gdp->mul;
		rv = 0;
	}

	return rv;
}
