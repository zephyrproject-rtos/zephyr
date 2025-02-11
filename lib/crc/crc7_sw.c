/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/crc.h>

uint8_t crc7_be(uint8_t seed, const uint8_t *src, size_t len)
{
	while (len-- != 0UL) {
		uint8_t e = seed ^ *src++;
		uint8_t f = e ^ (e >> 4) ^ (e >> 7);

		seed = (f << 1) ^ (f << 4);
	}

	return seed;
}
