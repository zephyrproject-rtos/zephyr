/*  NVS: non volatile storage in flash
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __NVS_PRIV_H_
#define __NVS_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MASKS AND SHIFT FOR ADDRESSES
 * an address in nvs is an uint32_t where:
 *   high 2 bytes represent the sector number
 *   low 2 bytes represent the offset in a sector
 */
#define ADDR_SECT_MASK 0xFFFF0000
#define ADDR_SECT_SHIFT 16
#define ADDR_OFFS_MASK 0x0000FFFF

/*
 * Status return values
 */
#define NVS_STATUS_NOSPACE 1

#define NVS_BLOCK_SIZE 32

#define NVS_LOOKUP_CACHE_NO_ADDR 0xFFFFFFFF

/*
 * Allow to use the NVS_DATA_CRC_SIZE macro in computations whether data CRC is enabled or not
 */
#ifdef CONFIG_NVS_DATA_CRC
#define NVS_DATA_CRC_SIZE 4 /* CRC-32 size in bytes */
#else
#define NVS_DATA_CRC_SIZE 0
#endif

/* Allocation Table Entry */
struct nvs_ate {
	uint16_t id;	/* data id */
	uint16_t offset;	/* data offset within sector */
	uint16_t len;	/* data len within sector */
	uint8_t part;	/* part of a multipart data - future extension */
	uint8_t crc8;	/* crc8 check of the entry */
} __packed;

BUILD_ASSERT(offsetof(struct nvs_ate, crc8) ==
		 sizeof(struct nvs_ate) - sizeof(uint8_t),
		 "crc8 must be the last member");

#ifdef __cplusplus
}
#endif

#endif /* __NVS_PRIV_H_ */
