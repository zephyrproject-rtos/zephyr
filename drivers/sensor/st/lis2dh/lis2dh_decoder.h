/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DH_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DH_DECODER_H_

#include <stdint.h>

struct lis2dh_encoded_header {
	uint64_t timestamp_ns;
	uint64_t period_ns;
	uint32_t scale;
	uint16_t sample_count;
	uint16_t trigger;
	int8_t shift;
	uint8_t is_fifo;
	uint8_t reserved[2];
} __packed;

#define LIS2DH_ENCODED_SAMPLE_SIZE 6U

static inline int8_t lis2dh_encoded_shift(uint32_t scale)
{
	uint64_t max_micro_ms2 = (uint64_t)scale * 2048U;
	int8_t shift = 0;

	while (max_micro_ms2 > (UINT64_C(1000000) << shift)) {
		shift++;
	}
	return shift;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_LIS2DH_DECODER_H_ */
