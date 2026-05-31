/*
 * Copyright (c) 2026 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "conversions.h"

float sensirion_conversions_bytes_to_float(const uint8_t *const bytes)
{
	union {
		uint32_t u32_value;
		float float32;
	} tmp;

	tmp.u32_value = sys_get_be32(bytes);
	return tmp.float32;
}

void sensirion_conversions_to_integer(const uint8_t *const source, uint8_t *const destination,
				      size_t destination_size, uint8_t data_length)
{
	if (data_length > destination_size) {
		memset(destination, 0xFF, destination_size);
		return;
	}
	/* set all bytes in destination to 0 to make sure that the value is correct even if the
	 * data_length is smaller than the size of the integer type
	 */
	memset(destination, 0, destination_size);

	/* copy the bytes from source to destination. The source is in big-endian/MSB-first format,
	 * so the first byte of the source is the most significant byte. The destination should be
	 * filled in the correct order for the system endianness.
	 */
	if (IS_ENABLED(CONFIG_LITTLE_ENDIAN)) {
		for (uint8_t i = 1; i <= data_length; i++) {
			destination[data_length - i] = source[i - 1u];
		}
	} else {
		memcpy(&destination[destination_size - data_length], source, data_length);
	}
}
