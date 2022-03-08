/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/crc.h>

uint16_t crc16(const uint8_t *src, size_t len, uint16_t polynomial,
	    uint16_t initial_value, bool pad)
{
	uint16_t crc = initial_value;
	size_t padding = pad ? sizeof(crc) : 0U;
	size_t i, b;

	/* src length + padding (if required) */
	for (i = 0; i < (len + padding); i++) {

		for (b = 0; b < 8; b++) {
			uint16_t divide = crc & 0x8000U;

			crc = (crc << 1U);

			/* choose input bytes or implicit trailing zeros */
			if ((i < len) && ((src[i] & (0x80U >> b)) != 0)) {
				crc |= 1U;
			}

			if (divide != 0U) {
				crc = crc ^ polynomial;
			}
		}
	}

	return crc;
}

uint16_t crc16_ccitt(uint16_t seed, const uint8_t *src, size_t len)
{
	for (; len > 0; len--) {
		uint8_t e, f;

		e = (uint8_t)(seed ^ *src);
		++src;
		f = e ^ (e << 4);
		seed = (seed >> 8) ^ ((uint16_t)f << 8) ^ ((uint16_t)f << 3) ^ ((uint16_t)f >> 4);
	}

	return seed;
}

uint16_t crc16_itu_t(uint16_t seed, const uint8_t *src, size_t len)
{
	for (; len > 0; len--) {
		seed = (seed >> 8U) | (seed << 8U);
		seed ^= *src;
		++src;
		seed ^= (seed & 0xffU) >> 4U;
		seed ^= seed << 12U;
		seed ^= (seed & 0xffU) << 5U;
	}

	return seed;
}
