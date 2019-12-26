/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/adc.h>

int adc_gain_invert(enum adc_gain gain,
		    s32_t *value)
{
	int rv = 0;

	switch (gain) {
	case ADC_GAIN_1_6:
		*value *= 6;
		break;
	case ADC_GAIN_1_5:
		*value *= 5;
		break;
	case ADC_GAIN_1_4:
		*value *= 4;
		break;
	case ADC_GAIN_1_3:
		*value *= 3;
		break;
	case ADC_GAIN_1_2:
		*value *= 2;
		break;
	case ADC_GAIN_2_3:
		*value = (3 * *value) / 2;
		break;
	case ADC_GAIN_1:
		break;
	case ADC_GAIN_2:
		*value /= 2;
		break;
	case ADC_GAIN_3:
		*value /= 3;
		break;
	case ADC_GAIN_4:
		*value /= 4;
		break;
	case ADC_GAIN_8:
		*value /= 8;
		break;
	case ADC_GAIN_16:
		*value /= 16;
		break;
	case ADC_GAIN_32:
		*value /= 32;
		break;
	case ADC_GAIN_64:
		*value /= 64;
		break;
	default:
		rv = -EINVAL;
		break;
	}

	return rv;
}

int adc_ref_vdd_invert(enum adc_reference ref,
		       s32_t *value)
{
	int rv = 0;

	switch (ref) {
	case ADC_REF_VDD_1:
		break;
	case ADC_REF_VDD_1_2:
		*value *= 2;
		break;
	case ADC_REF_VDD_1_3:
		*value *= 3;
		break;
	case ADC_REF_VDD_1_4:
		*value *= 4;
		break;
	case ADC_REF_INTERNAL:
	case ADC_REF_EXTERNAL0:
	case ADC_REF_EXTERNAL1:
		rv = -ENOTSUP;
		break;
	default:
		rv = -EINVAL;
		break;
	}

	return rv;
}
