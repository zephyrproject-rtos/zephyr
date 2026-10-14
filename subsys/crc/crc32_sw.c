/*
 * Copyright (c) 2018 Workaround GmbH.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/crc.h>

uint32_t __weak crc32_ieee(const uint8_t *data, size_t len)
{
	return crc32_ieee_update(0x0, data, len);
}

uint32_t __weak crc32_ieee_update(uint32_t crc, const uint8_t *data, size_t len)
{
	/* crc table generated from polynomial 0xedb88320 */
	static const uint32_t table[16] = {
		0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU, 0x76dc4190U, 0x6b6b51f4U,
		0x4db26158U, 0x5005713cU, 0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
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

uint32_t __weak crc32_mpeg2(const uint8_t *data, size_t len)
{
	return crc32_mpeg2_update(0xFFFFFFFFU, data, len);
}

uint32_t __weak crc32_mpeg2_update(uint32_t crc, const uint8_t *data, size_t len)
{
	/* crc table generated from polynomial 0x04C11DB7 (MSB-first, no reflection) */
	static const uint32_t table[16] = {
		0x00000000U, 0x04C11DB7U, 0x09823B6EU, 0x0D4326D9U, 0x130476DCU, 0x17C56B6BU,
		0x1A864DB2U, 0x1E475005U, 0x2608EDB8U, 0x22C9F00FU, 0x2F8AD6D6U, 0x2B4BCB61U,
		0x350C9B64U, 0x31CD86D3U, 0x3C8EA00AU, 0x384FBDBDU,
	};

	for (size_t i = 0; i < len; i++) {
		uint8_t byte = data[i];

		crc = (crc << 4) ^ table[((crc >> 28) ^ ((uint32_t)byte >> 4)) & 0x0f];
		crc = (crc << 4) ^ table[((crc >> 28) ^ byte) & 0x0f];
	}

	return crc;
}
