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
 *   high bits represent the sector number
 *   low bits represent the offset in a sector
 *   The number of offset bits is determined by
 *   NVS_OFFSET_BITS
 */
#if defined CONFIG_NVS_SECTOR_SIZE_64K
#define NVS_OFFSET_BITS 16
#elif defined CONFIG_NVS_SECTOR_SIZE_256K
#define NVS_OFFSET_BITS 18
#else
#error "No maximum sector size defined for NVS"
#endif

#define ADDR_SECT_MASK (0xFFFFFFFF << NVS_OFFSET_BITS)
#define ADDR_OFFS_MASK (0xFFFFFFFF >> (32 - NVS_OFFSET_BITS))

/*
 * Status return values
 */
#define NVS_STATUS_NOSPACE 1

#define NVS_BLOCK_SIZE 32

/* Allocation Table Entry */
struct nvs_ate {
#if NVS_OFFSET_BITS > 16
	/* Order to keep proper alignment within struct */
	uint32_t offset;   /* data offset within sector */
	uint16_t len;      /* data length */
	uint16_t id;       /* data id */
#else
	uint16_t id;       /* data id */
	uint16_t offset;   /* data offset within sector */
	uint16_t len;      /* data length */
#endif
	uint8_t part;      /* part of a multipart data - future extension */
	uint8_t crc8;      /* crc8 check of the entry */
} __packed;

BUILD_ASSERT(offsetof(struct nvs_ate, crc8) ==
		 sizeof(struct nvs_ate) - sizeof(uint8_t),
		 "crc8 must be the last member");

#ifdef __cplusplus
}
#endif

#endif /* __NVS_PRIV_H_ */
