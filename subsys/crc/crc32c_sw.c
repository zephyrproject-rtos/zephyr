/*
 * Copyright (c) 2021 Workaround GmbH.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/crc.h>

/* crc table generated from polynomial 0x1EDC6F41UL (Castagnoli) */
static const uint32_t crc32c_table[16] = {
	0x00000000UL, 0x105EC76FUL, 0x20BD8EDEUL, 0x30E349B1UL, 0x417B1DBCUL, 0x5125DAD3UL,
	0x61C69362UL, 0x7198540DUL, 0x82F63B78UL, 0x92A8FC17UL, 0xA24BB5A6UL, 0xB21572C9UL,
	0xC38D26C4UL, 0xD3D3E1ABUL, 0xE330A81AUL, 0xF36E6F75UL,
};

/* This value needs to be XORed with the final crc value once crc for
 * the entire stream is calculated. This is a requirement of crc32c algo.
 */
#define CRC32C_XOR_OUT 0xFFFFFFFFUL

/* The crc32c algorithm requires the below value as Init value at the
 * beginning of the stream.
 */
#define CRC32C_INIT 0xFFFFFFFFUL

uint32_t __weak crc32_c(uint32_t crc, const uint8_t *data, size_t len, bool first_pkt,
			bool last_pkt)
{
	if (first_pkt) {
		crc = CRC32C_INIT;
	}

	for (size_t i = 0; i < len; i++) {
		crc = crc32c_table[(crc ^ data[i]) & 0x0F] ^ (crc >> 4);
		crc = crc32c_table[(crc ^ ((uint32_t)data[i] >> 4)) & 0x0F] ^ (crc >> 4);
	}

	return last_pkt ? (crc ^ CRC32C_XOR_OUT) : crc;
}
