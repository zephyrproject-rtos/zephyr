/*
 * Copyright (c) 2018 Workaround GmbH.
 * Copyright (c) 2025 Arch-Embedded B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/crc.h>

uint32_t crc32_ieee(const uint8_t *data, size_t len)
{
	return crc32_ieee_update(0x0, data, len);
}

uint32_t crc32_ieee_update(uint32_t crc, const uint8_t *data, size_t len)
{
	/* crc table generated from polynomial 0xedb88320 */
	static const uint32_t table[16] = {
		0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU,
		0x76dc4190U, 0x6b6b51f4U, 0x4db26158U, 0x5005713cU,
		0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
		0x9b64c2b0U, 0x86d3d2d4U, 0xa00ae278U, 0xbdbdf21cU,
	};

	crc = ~crc;

	for (size_t i = 0; i < len; i++) {
		uint8_t byte = data[i];

		crc = (crc >> 4) ^ table[(crc ^ byte) & 0x0f];
		crc = (crc >> 4) ^ table[(crc ^ ((uint32_t)byte >> 4)) & 0x0f];
	}

	return (~crc);
}

uint32_t crc32_posix(const uint8_t *data, size_t len)
{
	return crc32_posix_update(0x0, data, len);
}

uint32_t crc32_posix_update(uint32_t crc, const uint8_t *data, size_t len)
{
	/**
	 * CRC table generated from polynomial 0x04c11db7
	 *
	 * pycrc --width 32 --poly 0x04c11db7 --reflect-in False --xor-in 0xFFFFFFFF
	 * --reflect-out False --xor-out 0 --generate table --table-idx-width 4
	 */
	static const uint32_t table[16] = {
		0x00000000U, 0x04c11db7U, 0x09823b6eU, 0x0d4326d9U,
		0x130476dcU, 0x17c56b6bU, 0x1a864db2U, 0x1e475005U,
		0x2608edb8U, 0x22c9f00fU, 0x2f8ad6d6U, 0x2b4bcb61U,
		0x350c9b64U, 0x31cd86d3U, 0x3c8ea00aU, 0x384fbdbdU,
	};

	for (size_t i = 0; i < len; i++) {
		crc = (crc << 4) ^ table[((crc >> 28) ^ (data[i] >> 4)) & 0x0f];
		crc = (crc << 4) ^ table[((crc >> 28) ^ data[i]) & 0x0f];
	}

	return (~crc);
}
