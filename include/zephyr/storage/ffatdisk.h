/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake FAT Disk public header
 *
 * Header exposes API for defining the files of a fake FAT disk.
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_FFATDISK
#define ZEPHYR_INCLUDE_STORAGE_FFATDISK

#include <stdint.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/iterable_sections.h>

/**
 * @brief Fake FAT Disk support
 * @defgroup ffat Fake FAT Disk support
 * @ingroup storage_apis
 * @since 4.6
 * @version 0.1.0
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 * Internally used
 */

#define FAT_FILE_NAME_LEN		11

struct ffat_file {
	sys_snode_t node;
	const char *disk_name;
	uint32_t size;
	uint32_t start;
	uint32_t end;
	int (*rd_cb)(struct ffat_file *const file, const uint32_t block,
		     uint8_t *const buf, const uint32_t size);
	int (*wr_cb)(struct ffat_file *const file, const uint32_t block,
		     const uint8_t *const buf, const uint32_t size);
	void const *priv;
	char name[FAT_FILE_NAME_LEN];
	uint8_t attr;
};

/** @endcond */

/**
 * @brief Macro to define the FFAT file and place it in the right memory section.
 *
 * The file appears in the root directory of the disk @p d_name and reserves
 * @p f_size bytes, rounded up to the cluster size of the disk. Its content is
 * never stored, the callbacks provide and consume it one block at a time, where
 * @p size is the sector size of the disk. Because the reserved space is rounded
 * up, the block index can address blocks beyond the file size.
 *
 * The read callback fills @p buf and returns 0 on success, other values on
 * error. The block index is relative to the start of the file. The buffer is
 * zeroed before the call, so a callback with nothing to provide can leave it
 * untouched.
 *
 * @code{.c}
 * static int foo_rd_cb(struct ffat_file *const f, const uint32_t block,
 *                      uint8_t *const buf, const uint32_t size)
 * {
 *     return foo_image_read(block * size, buf, size);
 * }
 * @endcode
 *
 * A disk can have at most one file with a write callback. It gets the host
 * writes to the clusters reserved for it, and also those to free clusters, so
 * that a payload the host places itself, for example an image copied onto the
 * volume, is delivered as well. Only in the former case is the block index
 * exact. Otherwise it is relative to the block the callback anchored the payload
 * on, which is reported as block 0, and as long as there is no anchor every
 * block is reported as block 0.
 *
 * The write callback returns a positive value if a block is the start of a
 * payload, which becomes the new anchor and ends a preceding payload, -ENODATA
 * if a block is not payload at all, so that host filesystem metadata in the free
 * space is not mistaken for one, otherwise 0 on success and a negative errno on
 * error.
 *
 * A host marks the volume dirty before it writes to it and clean again once it
 * has flushed everything, at the latest when it unmounts the volume. When the
 * host marks it clean, the write callback is called with @p buf set to NULL and
 * @p block and @p size set to zero, which means that a payload the host placed
 * is complete. This can happen more than once per payload.
 *
 * @code{.c}
 * static int foo_wr_cb(struct ffat_file *const f, const uint32_t block,
 *                      const uint8_t *const buf, const uint32_t size)
 * {
 *     if (buf == NULL) {
 *         return foo_image_flushed();
 *     }
 *
 *     if (!foo_payload_block(buf)) {
 *         return -ENODATA;
 *     }
 *
 *     if (foo_payload_start(buf)) {
 *         foo_image_reset();
 *         foo_image_write(0U, buf, size);
 *
 *         return 1;
 *     }
 *
 *     return foo_image_write(block * size, buf, size);
 * }
 * @endcode
 *
 * @param s_name  Symbol name of the file structure
 * @param d_name  Name of the disk the file is placed on, a file that matches no
 *                disk is ignored
 * @param f_name  File name in FAT 8.3 short name format, exactly 11 space
 *                padded characters and no terminating null character
 * @param f_size  File size in bytes
 * @param f_rd_cb Read callback, NULL if the file is not readable, in which case
 *                it is marked hidden
 * @param f_wr_cb Write callback, NULL if the file is not writable, in which case
 *                it is marked read only
 * @param f_priv  Pointer to private data, may be NULL
 */
#define FFAT_FILE_DEFINE(s_name, d_name, f_name, f_size, f_rd_cb, f_wr_cb, f_priv)	\
	BUILD_ASSERT(f_size <= UINT32_MAX,						\
		     "File size is greater than UINT32_MAX");				\
	static STRUCT_SECTION_ITERABLE(ffat_file, s_name) = {				\
		.name = f_name,								\
		.disk_name = d_name,							\
		.size = f_size,								\
		.rd_cb = f_rd_cb,							\
		.wr_cb = f_wr_cb,							\
		.priv = f_priv,								\
	}

/**
 * @}
 */

#endif	/* ZEPHYR_INCLUDE_STORAGE_FFATDISK */
