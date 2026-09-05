/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief File sink element for the MP fs plugin.
 *
 * Writes pipeline data to a file using Zephyr's filesystem API.
 */

#ifndef ZEPHYR_INCLUDE_MP_FS_MP_FILESINK_H_
#define ZEPHYR_INCLUDE_MP_FS_MP_FILESINK_H_

/**
 * @defgroup mp_fs_sinks Sinks
 * @ingroup mp_fs
 * @brief File-backed sink element.
 * @{
 */

#include <zephyr/fs/fs.h>

#include <zephyr/mp/mp_sink.h>

/**
 * @brief File sink property identifiers.
 *
 * Extends the base sink properties defined in @ref mp_prop_sink.
 */
enum mp_prop_fs_sink {
	/** Path to the output file (const char *). */
	MP_PROP_FS_SINK_PATH = MP_PROP_SINK_LAST,
};

/**
 * @brief File sink element.
 *
 * Extends the base @ref mp_sink to write received buffers to a file
 * on any Zephyr-supported filesystem.
 */
struct mp_filesink {
	/** Base sink element. */
	struct mp_sink sink;
	/** File handle. */
	struct fs_file_t file;
	/** Whether the file is currently open. */
	bool file_open;
	/** Path to the output file. */
	const char *path;
};

/**
 * @brief Initialize a file sink element.
 *
 * @param self Pointer to the element to initialize.
 */
void mp_filesink_init(struct mp_element *self);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_FS_MP_FILESINK_H_ */
