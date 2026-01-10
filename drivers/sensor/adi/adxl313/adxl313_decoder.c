/*
 * Copyright (c) 2025 Lothar Rubusch <l.rubusch@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adxl313.h"

/*
 * The q-scale factor will always be the same, as the nominal LSB/g
 * changes at the same rate the selected shift parameter per range:
 *
 * - At 0.5G: 256 LSB/g, 10-bits resolution.
 * - At 1g: 128 LSB/g, 10-bits resolution.
 * - At 2g: 64 LSB/g, 10-bits resolution.
 * - At 4g 32 LSB/g, 10-bits resolution.
 */
static const uint32_t qscale_factor_no_full_res[] = {
	[ADXL313_RANGE_0_5G] = UINT32_C(2570754),
	[ADXL313_RANGE_1G] = UINT32_C(2570754),
	[ADXL313_RANGE_2G] = UINT32_C(2570754),
	[ADXL313_RANGE_4G] = UINT32_C(2570754),
};

/*
 * Sensitivities based on Range:
 *
 * - At 0.5G: 256 LSB/g, 10-bits resolution.
 * - At 1g: 256 LSB/g, 11-bits resolution.
 * - At 2g: 256 LSB/g, 12-bits resolution.
 * - At 4g 256 LSB/g, 13-bits resolution.
 */
static const uint32_t qscale_factor_full_res[] = {
	[ADXL313_RANGE_0_5G] = UINT32_C(2570754),
	[ADXL313_RANGE_1G] = UINT32_C(1285377),
	[ADXL313_RANGE_2G] = UINT32_C(642688),
	[ADXL313_RANGE_4G] = UINT32_C(321344),
};

void adxl313_accel_convert_q31(q31_t *out, int16_t sample, enum adxl313_range range,
			       bool is_full_res)
{
	if (is_full_res) {
		switch (range) {
		case ADXL313_RANGE_0_5G:
			if (sample & BIT(9)) {
				sample |= ADXL313_COMPLEMENT_MASK(10);
			}
			break;
		case ADXL313_RANGE_1G:
			if (sample & BIT(10)) {
				sample |= ADXL313_COMPLEMENT_MASK(11);
			}
			break;
		case ADXL313_RANGE_2G:
			if (sample & BIT(11)) {
				sample |= ADXL313_COMPLEMENT_MASK(12);
			}
			break;
		case ADXL313_RANGE_4G:
			if (sample & BIT(12)) {
				sample |= ADXL313_COMPLEMENT_MASK(13);
			}
			break;
		}
		*out = sample * qscale_factor_full_res[range];
	} else {
		if (sample & BIT(9)) {
			sample |= ADXL313_COMPLEMENT;
		}
		*out = sample * qscale_factor_no_full_res[range];
	}
}
