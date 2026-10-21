/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/bitmap.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

/* Searches for a cleared bit by inverting every byte with invert = 0xff. */
static size_t find_next(const uint8_t *bitmap, size_t num_bits, size_t start, uint8_t invert)
{
	size_t last_byte;
	size_t byte;
	size_t offset;
	uint8_t bits;

	if (start >= num_bits) {
		return num_bits;
	}

	last_byte = (num_bits - 1U) / BITS_PER_BYTE;
	byte = start / BITS_PER_BYTE;
	bits = (uint8_t)((bitmap[byte] ^ invert) & (UINT8_MAX << (start % BITS_PER_BYTE)));

	while (bits == 0U) {
		if (byte == last_byte) {
			return num_bits;
		}

		byte++;
		bits = (uint8_t)(bitmap[byte] ^ invert);
	}

	offset = byte * BITS_PER_BYTE;

	/* A match in the unused bits of the last byte is no match. */
	return MIN((size_t)u32_count_trailing_zeros(bits), num_bits - offset) + offset;
}

size_t sys_bitmap_find_next_set(const uint8_t *bitmap, size_t num_bits, size_t start)
{
	return find_next(bitmap, num_bits, start, 0U);
}

size_t sys_bitmap_find_next_clear(const uint8_t *bitmap, size_t num_bits, size_t start)
{
	return find_next(bitmap, num_bits, start, UINT8_MAX);
}

size_t sys_bitmap_count_set(const uint8_t *bitmap, size_t num_bits)
{
	size_t full_bytes = num_bits / BITS_PER_BYTE;
	size_t count = sys_count_bits(bitmap, full_bytes);

	if ((num_bits % BITS_PER_BYTE) != 0U) {
		uint8_t tail = bitmap[full_bytes] & (uint8_t)BIT_MASK(num_bits % BITS_PER_BYTE);

		count += sys_count_bits(&tail, sizeof(tail));
	}

	return count;
}
