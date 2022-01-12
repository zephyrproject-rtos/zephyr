/*
 * Copyright (c) 2021 Workaround GmbH.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/crc.h>

/* crc table generated from polynomial 0x1EDC6F41UL (Castagnoli) */
static const uint32_t crc32c_table[16] = {
	0x00000000U, 0x105EC76FU, 0x20BD8EDEU, 0x30E349B1U,
	0x417B1DBCU, 0x5125DAD3U, 0x61C69362U, 0x7198540DU,
	0x82F63B78U, 0x92A8FC17U, 0xA24BB5A6U, 0xB21572C9U,
	0xC38D26C4U, 0xD3D3E1ABU, 0xE330A81AU, 0xF36E6F75U
};

/* This value needs to be XORed with the final crc value once crc for
 * the entire stream is calculated. This is a requirement of crc32c algo.
 */
#define CRC32C_XOR_OUT	0xFFFFFFFFU

/* The crc32c algorithm requires the below value as Init value at the
 * beginning of the stream.
 */
#define CRC32C_INIT	0xFFFFFFFFU

uint32_t crc32_c(uint32_t crc, const uint8_t *data,
		 size_t len, bool first_pkt, bool last_pkt)
{
	if (first_pkt) {
		crc = CRC32C_INIT;
	}

	for (size_t i = 0; i < len; i++) {
		crc = crc32c_table[(crc ^ data[i]) & 0x0FU] ^ (crc >> 4);
		crc = crc32c_table[(crc ^ ((uint32_t)data[i] >> 4)) & 0x0FU] ^ (crc >> 4);
	}

	return last_pkt ? (crc ^ CRC32C_XOR_OUT) : crc;
}
