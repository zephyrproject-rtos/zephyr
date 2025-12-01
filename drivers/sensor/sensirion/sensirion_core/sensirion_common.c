/*
 * Copyright (c) 2025 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sensirion_common.h"

float sensirion_common_bytes_to_float(const uint8_t *bytes)
{
	union {
		uint32_t u32_value;
		float float32;
	} tmp;

	tmp.u32_value = sys_get_be32(bytes);
	return tmp.float32;
}

void sensirion_common_float_to_bytes(const float value, uint8_t *bytes)
{
	union {
		uint32_t u32_value;
		float float32;
	} tmp;
	tmp.float32 = value;
	sys_put_be32(tmp.u32_value, bytes);
}

void sensirion_common_to_integer(const uint8_t *source, uint8_t *destination, INT_TYPE int_type,
				 uint8_t data_length)
{
	if (data_length > int_type) {
		data_length = 0;
	}

	uint8_t offset = int_type - data_length;

	for (uint8_t i = 0; i < offset; i++) {
		destination[int_type - i - 1] = 0;
	}

	for (uint8_t i = 1; i <= data_length; i++) {
		destination[int_type - offset - i] = source[i - 1];
	}
}
