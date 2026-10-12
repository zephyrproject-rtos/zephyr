/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief File sink element for the mpipe fs plugin.
 * @ingroup mpipe_fs_sinks
 *
 * Writes pipeline data to a file using Zephyr's filesystem API.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_FS_MPIPE_FILE_SINK_H_
#define ZEPHYR_INCLUDE_MPIPE_FS_MPIPE_FILE_SINK_H_

/**
 * @defgroup mpipe_fs_sinks Sinks
 * @ingroup mpipe_fs
 * @brief File-backed sink element.
 * @{
 */

#include <zephyr/fs/fs.h>

#include <zephyr/mpipe/mpipe_sink.h>

/**
 * @brief File sink property identifiers.
 *
 * Extends the base sink properties defined in @ref mpipe_prop_sink.
 */
enum mpipe_prop_fs_sink {
	/** Path to the output file (const char *). */
	MPIPE_PROP_FS_SINK_PATH = MPIPE_PROP_SINK_LAST,
};

/**
 * @brief File sink element.
 *
 * Extends the base @ref mpipe_sink to write received buffers to a file
 * on any Zephyr-supported filesystem.
 */
struct mpipe_file_sink {
	/** Base sink element. */
	struct mpipe_sink sink;
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
 * @param fsink Element to initialize.
 * @param id    Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_file_sink_init(struct mpipe_file_sink *fsink, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_FS_MPIPE_FILE_SINK_H_ */
