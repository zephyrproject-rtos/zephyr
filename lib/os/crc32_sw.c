/*
 * Copyright (c) 2018 Workaround GmbH.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/crc.h>

uint32_t crc32_ieee(const uint8_t *data, size_t len)
{
	return crc32_ieee_update(0x0, data, len);
}

uint32_t crc32_ieee_update(uint32_t crc, const uint8_t *data, size_t len)
{
	crc = ~crc;
	for (size_t i = 0; i < len; i++) {
		crc = crc ^ data[i];

		for (uint8_t j = 0; j < 8; j++) {
			crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
		}
	}

	return (~crc);
}
