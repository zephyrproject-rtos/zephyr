/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Transport-neutral partition table parsing on block devices
 *
 * Parses MBR (including extended/logical) and GPT layout on a whole disk
 * using a sector read callback (DOS MBR, extended, and GPT layout). Filesystem
 * identification is separate; see
 * @ref disk_partition_find_fat_volume and @ref disk_partition_find_ext2_volume.
 *
 * When @ref disk_partition_walk returns @ref DISK_PARTITION_SCHEME_NONE with
 * no entries, the caller may treat the whole disk as one span via
 * @ref disk_partition_set_whole_disk.
 *
 * @since 4.5
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_DISK_PARTITION_H_
#define ZEPHYR_INCLUDE_STORAGE_DISK_PARTITION_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum logical block size supported by partition helpers */
#define DISK_PARTITION_MAX_BLOCK_SIZE 4096U

/** Maximum partition slots stored from one table walk (GPT may define more) */
#define DISK_PARTITION_MAX_ENTRIES 32U

/** MBR types for extended partition containers (not listed as data slices) */
#define DISK_PARTITION_MBR_EXTENDED     0x05U
#define DISK_PARTITION_MBR_EXTENDED_LBA 0x0FU /*!< MBR extended partition (LBA) */
#define DISK_PARTITION_MBR_EXTENDED_W2K 0x85U /*!< Windows extended partition type */

/** Whole-disk geometry passed to partition discovery */
struct disk_partition_geo {
	/** Logical block size in bytes (512–4096) */
	uint32_t block_size;
	/** Total sector count on the whole device */
	uint64_t sector_count;
};

/** Partition table format detected on the whole disk */
enum disk_partition_scheme {
	/** No MBR/GPT table was recognized */
	DISK_PARTITION_SCHEME_NONE = 0,
	/** DOS MBR (primary and/or logical partitions) */
	DISK_PARTITION_SCHEME_MBR,
	/** UEFI GPT (protective MBR or direct GPT header) */
	DISK_PARTITION_SCHEME_GPT,
	/** Synthetic single span for the whole disk (@ref disk_partition_set_whole_disk) */
	DISK_PARTITION_SCHEME_WHOLE_DISK,
};

/** One partition slot on the whole disk (start LBA on parent device) */
struct disk_partition_entry {
	/** First logical block of the partition on the whole disk */
	uint64_t start_lba;
	/** Length in logical blocks (0 means unknown; clamp against geometry) */
	uint64_t nr_sectors;
	/** MBR partition type byte (0 when @a type_guid is used) */
	uint8_t mbr_type;
	/** GPT partition type GUID (all zero for MBR-only entries) */
	uint8_t type_guid[16];
};

/** Parsed partition table from one walk of the whole disk */
struct disk_partition_table {
	/** Detected on-disk partition table format */
	enum disk_partition_scheme scheme;
	/** Number of populated @a entries */
	uint8_t entry_count;
	/** Partition slots read from the table */
	struct disk_partition_entry entries[DISK_PARTITION_MAX_ENTRIES];
};

/**
 * @brief Read one logical block for partition discovery
 *
 * @param ctx Opaque context from the partition helper
 * @param lba Logical block address on the whole device
 * @param buf Destination buffer (@a bytes long)
 * @param bytes Length to read (typically @c geo.block_size)
 *
 * @return 0 on success, negative errno on failure
 */
typedef int (*disk_partition_read_sectors_t)(void *ctx, uint64_t lba, void *buf, uint32_t bytes);

/**
 * @brief Parse MBR or GPT partition tables on a block device
 *
 * Reads LBA 0 and, when needed, extended boot records and the GPT entry
 * array. Stores up to @ref DISK_PARTITION_MAX_ENTRIES slices (extra GPT
 * entries are dropped with @c -ENOSPC). The caller supplies @a table; keep
 * roughly @c sizeof(struct disk_partition_entry) * @ref DISK_PARTITION_MAX_ENTRIES
 * bytes available on the stack when calling filesystem helpers that embed a table.
 *
 * @param geo Device geometry
 * @param read Sector read callback
 * @param read_ctx Context for @a read
 * @param table Output table (scheme may be @ref DISK_PARTITION_SCHEME_NONE)
 *
 * @retval 0 Table parsed or no recognized layout
 * @retval -EINVAL Invalid argument or on-disk metadata
 * @retval Negative errno Read failure from @a read
 */
int disk_partition_walk(const struct disk_partition_geo *geo, disk_partition_read_sectors_t read,
			void *read_ctx, struct disk_partition_table *table);

/**
 * @brief Expose the whole disk as a single partition entry
 *
 * Used when @ref disk_partition_walk finds no table (whole-disk access).
 *
 * @param geo Device geometry
 * @param table Table to fill
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid argument
 */
int disk_partition_set_whole_disk(const struct disk_partition_geo *geo,
				  struct disk_partition_table *table);

/**
 * @brief Find the first FAT/exFAT volume on block media
 *
 * Superfloppy media (FAT BPB at LBA 0) is handled first. Otherwise uses
 * @ref disk_partition_walk and checks each partition's boot sector for a
 * FAT/exFAT BPB.
 */
int disk_partition_find_fat_volume(const struct disk_partition_geo *geo,
				   disk_partition_read_sectors_t read, void *read_ctx,
				   uint64_t *lba_offset, uint64_t *sector_count);

/**
 * @brief Find the first ext2-compatible volume on block media
 *
 * Uses @ref disk_partition_walk (or whole-disk span when empty) and checks
 * the ext2 superblock magic at byte offset 1080 from each slice start
 * (1024-byte block size layout).
 */
int disk_partition_find_ext2_volume(const struct disk_partition_geo *geo,
				    disk_partition_read_sectors_t read, void *read_ctx,
				    uint64_t *lba_offset, uint64_t *sector_count);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_STORAGE_DISK_PARTITION_H_ */
