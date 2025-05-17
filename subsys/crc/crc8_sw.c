/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/crc.h>

static const uint8_t crc8_ccitt_small_table[16] = {
	0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
	0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
};

static const uint8_t crc8_rohc_small_table[16] = {
	0x00, 0x1c, 0x38, 0x24, 0x70, 0x6c, 0x48, 0x54,
	0xe0, 0xfc, 0xd8, 0xc4, 0x90, 0x8c, 0xa8, 0xb4,
};

uint8_t __weak crc8_ccitt(uint8_t val, const void *buf, size_t cnt)
{
	size_t i;
	const uint8_t *p = buf;

	for (i = 0; i < cnt; i++) {
		val ^= p[i];
		val = (val << 4) ^ crc8_ccitt_small_table[val >> 4];
		val = (val << 4) ^ crc8_ccitt_small_table[val >> 4];
	}
	return val;
}

uint8_t __weak crc8_rohc(uint8_t val, const void *buf, size_t cnt)
{
	size_t i;
	const uint8_t *p = buf;

	for (i = 0; i < cnt; i++) {
		val ^= p[i];
		val = (val >> 4) ^ crc8_rohc_small_table[val & 0x0f];
		val = (val >> 4) ^ crc8_rohc_small_table[val & 0x0f];
	}
	return val;
}

uint8_t __weak crc8(const uint8_t *src, size_t len, uint8_t polynomial, uint8_t initial_value,
		    bool reversed)
{
	uint8_t crc = initial_value;
	size_t i, j;

	for (i = 0; i < len; i++) {
		crc ^= src[i];

		for (j = 0; j < 8; j++) {
			if (reversed) {
				if ((crc & 0x01) != 0) {
					crc = (crc >> 1) ^ polynomial;
				} else {
					crc >>= 1;
				}
			} else {
				if ((crc & 0x80) != 0) {
					crc = (crc << 1) ^ polynomial;
				} else {
					crc <<= 1;
				}
			}
		}
	}

	return crc;
}
