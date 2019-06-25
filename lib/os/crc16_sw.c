/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/crc.h>

u16_t crc16(const u8_t *src, size_t len, u16_t polynomial,
	    u16_t initial_value, bool pad)
{
	u16_t crc = initial_value;
	size_t padding = pad ? sizeof(crc) : 0;
	size_t i, b;

	/* src length + padding (if required) */
	for (i = 0; i < len + padding; i++) {

		for (b = 0; b < 8; b++) {
			u16_t divide = crc & 0x8000UL;

			crc = (crc << 1U);

			/* choose input bytes or implicit trailing zeros */
			if (i < len) {
				crc |= !!(src[i] & (0x80U >> b));
			}

			if (divide != 0U) {
				crc = crc ^ polynomial;
			}
		}
	}

	return crc;
}

u16_t crc16_ccitt(u16_t seed, const u8_t *src, size_t len)
{
	for (; len > 0; len--) {
		u8_t e, f;

		e = seed ^ *src++;
		f = e ^ (e << 4);
		seed = (seed >> 8) ^ (f << 8) ^ (f << 3) ^ (f >> 4);
	}

	return seed;
}

u16_t crc16_itu_t(u16_t seed, const u8_t *src, size_t len)
{
	for (; len > 0; len--) {
		seed = (seed >> 8U) | (seed << 8U);
		seed ^= *src++;
		seed ^= (seed & 0xffU) >> 4U;
		seed ^= seed << 12U;
		seed ^= (seed & 0xffU) << 5U;
	}

	return seed;
}
