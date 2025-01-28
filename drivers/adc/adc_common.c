/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>

int adc_gain_invert(enum adc_gain gain,
		    int32_t *value)
{
	struct gain_desc {
		uint8_t mul;
		uint8_t div;
	};
	static const struct gain_desc gains[] = {
		[ADC_GAIN_1_6] = {.mul = 6, .div = 1},
		[ADC_GAIN_1_5] = {.mul = 5, .div = 1},
		[ADC_GAIN_1_4] = {.mul = 4, .div = 1},
		[ADC_GAIN_2_7] = {.mul = 7, .div = 2},
		[ADC_GAIN_1_3] = {.mul = 3, .div = 1},
		[ADC_GAIN_2_5] = {.mul = 5, .div = 2},
		[ADC_GAIN_1_2] = {.mul = 2, .div = 1},
		[ADC_GAIN_2_3] = {.mul = 3, .div = 2},
		[ADC_GAIN_4_5] = {.mul = 5, .div = 4},
		[ADC_GAIN_1] = {.mul = 1, .div = 1},
		[ADC_GAIN_2] = {.mul = 1, .div = 2},
		[ADC_GAIN_3] = {.mul = 1, .div = 3},
		[ADC_GAIN_4] = {.mul = 1, .div = 4},
		[ADC_GAIN_6] = {.mul = 1, .div = 6},
		[ADC_GAIN_8] = {.mul = 1, .div = 8},
		[ADC_GAIN_12] = {.mul = 1, .div = 12},
		[ADC_GAIN_16] = {.mul = 1, .div = 16},
		[ADC_GAIN_24] = {.mul = 1, .div = 24},
		[ADC_GAIN_32] = {.mul = 1, .div = 32},
		[ADC_GAIN_64] = {.mul = 1, .div = 64},
		[ADC_GAIN_128] = {.mul = 1, .div = 128},
	};
	int rv = -EINVAL;

	if ((uint8_t)gain < ARRAY_SIZE(gains)) {
		const struct gain_desc *gdp = &gains[gain];

		if ((gdp->mul != 0) && (gdp->div != 0)) {
			*value = (gdp->mul * *value) / gdp->div;
			rv = 0;
		}
	}

	return rv;
}
