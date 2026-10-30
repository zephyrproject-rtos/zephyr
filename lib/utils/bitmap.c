/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/bitmap.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

#if ULONG_MAX > UINT32_MAX
#define le_ulong_to_cpu(w)            sys_le64_to_cpu(w)
#define ulong_count_trailing_zeros(w) u64_count_trailing_zeros(w)
#else
#define le_ulong_to_cpu(w)            sys_le32_to_cpu(w)
#define ulong_count_trailing_zeros(w) u32_count_trailing_zeros(w)
#endif

/* A match in the unused bits of the last byte is no match. */
static size_t match(size_t byte, unsigned int bit, size_t num_bits)
{
	return MIN((byte * BITS_PER_BYTE) + bit, num_bits);
}

/* Searches for a cleared bit by inverting every byte with invert = 0xff. */
static size_t find_next(const uint8_t *bitmap, size_t num_bits, size_t start, uint8_t invert)
{
	const size_t num_bytes = SYS_BITMAP_SIZE(num_bits);
	const unsigned long word_invert = (invert != 0U) ? ~0UL : 0UL;
	size_t byte;
	uint8_t bits;

	if (start >= num_bits) {
		return num_bits;
	}

	byte = start / BITS_PER_BYTE;
	bits = (uint8_t)((bitmap[byte] ^ invert) & (UINT8_MAX << (start % BITS_PER_BYTE)));
	if (bits != 0U) {
		return match(byte, u32_count_trailing_zeros(bits), num_bits);
	}

	/* Bytes up to a word boundary */
	for (byte++; (byte < num_bytes) && !IS_ALIGNED(&bitmap[byte], sizeof(unsigned long));
	     byte++) {
		bits = (uint8_t)(bitmap[byte] ^ invert);
		if (bits != 0U) {
			return match(byte, u32_count_trailing_zeros(bits), num_bits);
		}
	}

	/* Whole words: bit n of a little-endian word is bit n % 8 of byte n / 8 */
	for (; (num_bytes - byte) >= sizeof(unsigned long); byte += sizeof(unsigned long)) {
		unsigned long word =
			le_ulong_to_cpu(*(const unsigned long *)&bitmap[byte]) ^ word_invert;

		if (word != 0UL) {
			return match(byte, ulong_count_trailing_zeros(word), num_bits);
		}
	}

	/* Remaining bytes */
	for (; byte < num_bytes; byte++) {
		bits = (uint8_t)(bitmap[byte] ^ invert);
		if (bits != 0U) {
			return match(byte, u32_count_trailing_zeros(bits), num_bits);
		}
	}

	return num_bits;
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
