/*
 * Copyright (c) 2025 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sensirion_common.h"

uint16_t sensirion_common_bytes_to_uint16_t(const uint8_t *bytes)
{
	return (uint16_t)bytes[0] << 8 | (uint16_t)bytes[1];
}

uint32_t sensirion_common_bytes_to_uint32_t(const uint8_t *bytes)
{
	return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 | (uint32_t)bytes[2] << 8 |
	       (uint32_t)bytes[3];
}

int16_t sensirion_common_bytes_to_int16_t(const uint8_t *bytes)
{
	return (int16_t)sensirion_common_bytes_to_uint16_t(bytes);
}

int32_t sensirion_common_bytes_to_int32_t(const uint8_t *bytes)
{
	return (int32_t)sensirion_common_bytes_to_uint32_t(bytes);
}

float sensirion_common_bytes_to_float(const uint8_t *bytes)
{
	union {
		uint32_t u32_value;
		float float32;
	} tmp;

	tmp.u32_value = sensirion_common_bytes_to_uint32_t(bytes);
	return tmp.float32;
}

void sensirion_common_uint32_t_to_bytes(const uint32_t value, uint8_t *bytes)
{
	bytes[0] = (uint8_t)(value >> 24);
	bytes[1] = (uint8_t)(value >> 16);
	bytes[2] = (uint8_t)(value >> 8);
	bytes[3] = (uint8_t)(value);
}

void sensirion_common_uint16_t_to_bytes(const uint16_t value, uint8_t *bytes)
{
	bytes[0] = (uint8_t)(value >> 8);
	bytes[1] = (uint8_t)value;
}

void sensirion_common_int32_t_to_bytes(const int32_t value, uint8_t *bytes)
{
	bytes[0] = (uint8_t)(value >> 24);
	bytes[1] = (uint8_t)(value >> 16);
	bytes[2] = (uint8_t)(value >> 8);
	bytes[3] = (uint8_t)value;
}

void sensirion_common_int16_t_to_bytes(const int16_t value, uint8_t *bytes)
{
	bytes[0] = (uint8_t)(value >> 8);
	bytes[1] = (uint8_t)value;
}

void sensirion_common_float_to_bytes(const float value, uint8_t *bytes)
{
	union {
		uint32_t u32_value;
		float float32;
	} tmp;
	tmp.float32 = value;
	sensirion_common_uint32_t_to_bytes(tmp.u32_value, bytes);
}

void sensirion_common_copy_bytes(const uint8_t *source, uint8_t *destination, uint16_t data_length)
{
	uint16_t i;

	for (i = 0; i < data_length; i++) {
		destination[i] = source[i];
	}
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
