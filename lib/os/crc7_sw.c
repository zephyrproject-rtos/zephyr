/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/crc.h>

u8_t crc7_be(u8_t seed, const u8_t *src, size_t len)
{
	while (len--) {
		u8_t e = seed ^ *src++;
		u8_t f = e ^ (e >> 4) ^ (e >> 7);

		seed = (f << 1) ^ (f << 4);
	}

	return seed;
}
