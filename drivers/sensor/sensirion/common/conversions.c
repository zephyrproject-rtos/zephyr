/*
 * Copyright (c) 2025 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "conversions.h"

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
		data_length = int_type;
	}
	// set all bytes in destination to 0 to make sure that the value is correct even if the data_length is smaller than the size of the integer type
	memset(destination, 0, data_length);
	uint8_t offset = int_type - data_length;

	// copy the bytes from source to destination. The source is in big-endian/MSB-first format, so the first byte of the source is the most significant byte.
	// The destination should be filled in the correct order for the system endianness.
	for (uint8_t i = 1; i <= data_length; i++) {
		destination[int_type - offset - i] = source[i - 1];
	}
}
