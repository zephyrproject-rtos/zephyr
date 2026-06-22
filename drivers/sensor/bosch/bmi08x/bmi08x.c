/* Bosch BMI08X inertial measurement unit driver
 *
 * Copyright (c) 2022 Meta Platforms, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "bmi08x.h"

/*
 * Output data rate map with allowed frequencies:
 * freq = freq_int + freq_milli / 1000
 *
 * Since we don't need a finer frequency resolution than milliHz, use uint16_t
 * to save some flash.
 */
static const struct {
	uint16_t freq_int;
	uint16_t freq_milli; /* User should convert to uHz before setting the
			      * SENSOR_ATTR_SAMPLING_FREQUENCY attribute.
			      */
} bmi08x_odr_map[] = {
	{0, 0},	 {0, 780}, {1, 562}, {3, 120}, {6, 250}, {12, 500}, {25, 0},
	{50, 0}, {100, 0}, {200, 0}, {400, 0}, {800, 0}, {1600, 0}, {3200, 0},
};

int bmi08x_freq_to_odr_val(uint16_t freq_int, uint16_t freq_milli)
{
	size_t i;

	/* An ODR of 0 Hz is not allowed */
	if (freq_int == 0U && freq_milli == 0U) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(bmi08x_odr_map); i++) {
		if (freq_int < bmi08x_odr_map[i].freq_int ||
		    (freq_int == bmi08x_odr_map[i].freq_int &&
		     freq_milli <= bmi08x_odr_map[i].freq_milli)) {
			return i;
		}
	}

	return -EINVAL;
}

int32_t bmi08x_range_to_reg_val(uint16_t range, const struct bmi08x_range *range_map,
				       uint16_t range_map_size)
{
	int i;

	for (i = 0; i < range_map_size; i++) {
		if (range <= range_map[i].range) {
			return range_map[i].reg_val;
		}
	}

	return -EINVAL;
}

int32_t bmi08x_reg_val_to_range(uint8_t reg_val, const struct bmi08x_range *range_map,
				       uint16_t range_map_size)
{
	int i;

	for (i = 0; i < range_map_size; i++) {
		if (reg_val == range_map[i].reg_val) {
			return range_map[i].range;
		}
	}

	return -EINVAL;
}
