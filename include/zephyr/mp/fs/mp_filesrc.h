/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief File source element for the MP fs plugin.
 *
 * Reads data from a file using Zephyr's filesystem API and produces
 * buffers for downstream processing.
 */

#ifndef ZEPHYR_INCLUDE_MP_FS_MP_FILESRC_H_
#define ZEPHYR_INCLUDE_MP_FS_MP_FILESRC_H_

/**
 * @defgroup mp_fs fs
 * @ingroup mp_plugins
 * @brief File-based source and sink elements.
 */

/**
 * @defgroup mp_fs_sources Sources
 * @ingroup mp_fs
 * @brief File-backed source elements.
 * @{
 */

#include <zephyr/fs/fs.h>

#include <zephyr/mp/mp_src.h>

/**
 * @brief File source property identifiers.
 *
 * Extends the base source properties defined in @ref mp_prop_src.
 */
enum mp_prop_fs_src {
	/** Path to the input file (const char *). */
	MP_PROP_FS_SRC_PATH = MP_PROP_SRC_LAST,
	/** Read block size in bytes (uint32_t). */
	MP_PROP_FS_SRC_BLOCKSIZE,
};

/**
 * @brief File source element.
 *
 * Extends the base @ref mp_src to read data from a file on any
 * Zephyr-supported filesystem.
 */
struct mp_filesrc {
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
void mp_filesrc_init(struct mp_element *self);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_FS_MP_FILESRC_H_ */
