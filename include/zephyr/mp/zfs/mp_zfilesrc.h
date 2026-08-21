/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief File source element for the MP zfs plugin.
 *
 * Reads data from a file using Zephyr's filesystem API and produces
 * buffers for downstream processing.
 */

#ifndef ZEPHYR_INCLUDE_MP_ZFS_MP_ZFILESRC_H_
#define ZEPHYR_INCLUDE_MP_ZFS_MP_ZFILESRC_H_

#include <zephyr/fs/fs.h>

#include <zephyr/mp/core/mp_property.h>
#include <zephyr/mp/core/mp_src.h>

/**
 * @brief File source property identifiers.
 *
 * Extends the base source properties defined in @ref mp_property.h.
 */
enum prop_zfilesrc {
	/** Path to the input file (const char *). */
	PROP_ZFILESRC_PATH = PROP_SRC_LAST + 1,
	/** Read block size in bytes (uint32_t). */
	PROP_ZFILESRC_BLOCKSIZE,
};

/**
 * @brief File source element.
 *
 * Extends the base @ref mp_src to read data from a file on any
 * Zephyr-supported filesystem.
 */
struct mp_zfilesrc {
	/** Base source element. */
	struct mp_src src;
	/** Buffer pool used by this source. */
	struct mp_buffer_pool pool;
	/** Downstream-proposed buffer pool (may be NULL). */
	struct mp_buffer_pool *downstream_pool;
	/** File handle. */
	struct fs_file_t file;
	/** Whether the file is currently open. */
	bool file_open;
	/** Path to the input file. */
	const char *path;
	/** Read chunk size in bytes. */
	uint32_t blocksize;
};

/**
 * @brief Initialize a file source element.
 *
 * @param self Pointer to the element to initialize.
 */
void mp_zfilesrc_init(struct mp_element *self);

#endif /* ZEPHYR_INCLUDE_MP_ZFS_MP_ZFILESRC_H_ */
