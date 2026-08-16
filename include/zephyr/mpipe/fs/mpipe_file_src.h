/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief File source element for the mpipe fs plugin.
 * @ingroup mpipe_fs_sources
 *
 * Reads data from a file using Zephyr's filesystem API and produces
 * buffers for downstream processing.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_FS_MPIPE_FILE_SRC_H_
#define ZEPHYR_INCLUDE_MPIPE_FS_MPIPE_FILE_SRC_H_

/**
 * @defgroup mpipe_fs File System
 * @ingroup mpipe_plugins
 * @brief Elements that read from and write to a mounted file system.
 *
 * The file system plugin puts a file at either end of a graph: a source that
 * reads one in fixed-size blocks, and a sink that writes whatever reaches it.
 *
 * Neither knows what it is carrying, which is the point. A file is a stream of
 * bytes with no format attached, so a file source has nothing to announce and
 * negotiates no capability of its own; whatever follows it - typically a parser
 * - is what works out the format. That makes these two elements the usual way
 * to test a graph without hardware.
 */

/**
 * @defgroup mpipe_fs_sources Sources
 * @ingroup mpipe_fs
 * @brief File-backed source elements.
 * @{
 */

#include <zephyr/fs/fs.h>

#include <zephyr/mpipe/mpipe_src.h>

/**
 * @brief File source property identifiers.
 *
 * Extends the base source properties defined in @ref mpipe_prop_src.
 */
enum mpipe_prop_fs_src {
	/** Path to the input file (const char *). */
	MPIPE_PROP_FS_SRC_PATH = MPIPE_PROP_SRC_LAST,
	/** Read block size in bytes (uint32_t). */
	MPIPE_PROP_FS_SRC_BLOCKSIZE,
};

/**
 * @brief File source element.
 *
 * Extends the base @ref mpipe_src to read data from a file on any
 * Zephyr-supported filesystem.
 */
struct mpipe_file_src {
	/** Base source element. */
	struct mpipe_src src;
	/** Buffer pool used by this source. */
	struct mpipe_buffer_pool pool;
	/** Downstream-proposed buffer pool (may be NULL). */
	struct mpipe_buffer_pool *downstream_pool;
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
 * @param fsrc Element to initialize.
 * @param id   Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_file_src_init(struct mpipe_file_src *fsrc, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_FS_MPIPE_FILE_SRC_H_ */
