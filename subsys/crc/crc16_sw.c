/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/crc.h>

uint16_t __weak crc16(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len)
{
	uint16_t crc = seed;
	size_t i, j;

	for (i = 0; i < len; i++) {
		crc ^= (uint16_t)(src[i] << 8U);

		for (j = 0; j < 8; j++) {
			if (crc & 0x8000UL) {
				crc = (crc << 1U) ^ poly;
			} else {
				crc = crc << 1U;
			}
		}
	}

	return crc;
}

uint16_t __weak crc16_reflect(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len)
{
	uint16_t crc = seed;
	size_t i, j;

	for (i = 0; i < len; i++) {
		crc ^= (uint16_t)src[i];

		for (j = 0; j < 8; j++) {
			if (crc & 0x0001UL) {
				crc = (crc >> 1U) ^ poly;
			} else {
				crc = crc >> 1U;
			}
		}
	}

	return crc;
}

uint16_t __weak crc16_ccitt(uint16_t seed, const uint8_t *src, size_t len)
{
	for (; len > 0; len--) {
		uint8_t e, f;

		e = seed ^ *src;
		++src;
		f = e ^ (e << 4);
		seed = (seed >> 8) ^ ((uint16_t)f << 8) ^ ((uint16_t)f << 3) ^ ((uint16_t)f >> 4);
	}

	return seed;
}

uint16_t __weak crc16_itu_t(uint16_t seed, const uint8_t *src, size_t len)
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
