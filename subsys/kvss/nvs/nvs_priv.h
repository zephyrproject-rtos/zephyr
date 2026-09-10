/*  NVS: non volatile storage in flash
 *
 * Copyright (c) 2018 Laczen
 * Copyright (c) 2026 Lingao Meng
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

#define NVS_MAX_SECTOR_SIZE KB(64)

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

/* Context structure for NVS flash block move operations. */
struct nvs_block_move_ctx {
	/* Temporary buffer of size NVS_BLOCK_SIZE used to read
	 * and write flash data in write_block_size-aligned chunks.
	 */
	uint8_t buffer[NVS_BLOCK_SIZE];

	/* Number of bytes currently stored in the buffer that
	 * have not yet been written. Always < write_block_size.
	 */
	size_t buffer_pos;
};

/**
 * @brief Simple flash buffer descriptor
 *
 * Describes a single contiguous memory region to be written to flash.
 */
struct nvs_flash_buf {
	const uint8_t *ptr; /**< Pointer to buffer */
	size_t len;         /**< Length of buffer */
};

/**
 * @brief Description of a contiguous flash write
 *
 * Describes a flash write consisting of up to three buffers:
 *  - optional head
 *  - primary data
 *  - optional tail
 *
 * Buffers are written sequentially as a single logical stream,
 * respecting flash write-block alignment.
 */
struct nvs_flash_wrt_stream {
	struct nvs_flash_buf head; /**< Optional head buffer */
	struct nvs_flash_buf data; /**< Primary data buffer */
	struct nvs_flash_buf tail; /**< Optional tail buffer */
};

/**
 * @brief NVS GC Write entry context
 *
 * Holds data being written during garbage collection.
 */
struct nvs_gc_write_entry {
	uint16_t id;       /**< Entry ID */
	const void *data;  /**< Pointer to data */
	uint16_t len;      /**< Data length in bytes */
	bool is_written;   /**< GC written flags for upper layers */
};

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

#if defined(CONFIG_ZTEST)
int nvs_flash_al_wrt_streams(struct nvs_fs *fs, uint32_t addr,
			     const struct nvs_flash_wrt_stream *strm);
#endif /* CONFIG_ZTEST */

#ifdef __cplusplus
}
#endif

#endif /* __NVS_PRIV_H_ */
