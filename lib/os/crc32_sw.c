/*
 * Copyright (c) 2018 Workaround GmbH.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/crc.h>

u32_t crc32_ieee(const u8_t *data, size_t len)
{
	return crc32_ieee_update(0x0, data, len);
}

u32_t crc32_ieee_update(u32_t crc, const u8_t *data, size_t len)
{
	crc = ~crc;
	for (size_t i = 0; i < len; i++) {
		crc = crc ^ data[i];

		for (u8_t j = 0; j < 8; j++) {
			crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
		}
	}

	return (~crc);
}
