/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_FS_LOADER_H
#define ZEPHYR_LLEXT_FS_LOADER_H

#include <zephyr/llext/loader.h>
#include <zephyr/fs/fs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief LLEXT filesystem loader implementation.
 *
 * @addtogroup llext_loader_apis
 * @{
 */

/**
 * @brief Implementation of @ref llext_loader that reads from a filesystem.
 */
struct llext_fs_loader {
	/** Extension loader */
	struct llext_loader loader;

	/** @cond ignore */
	bool is_open;
	const char *name;
	struct fs_file_t file;
	/** @endcond */
};

/** @cond ignore */
int llext_fs_prepare(struct llext_loader *ldr);
int llext_fs_read(struct llext_loader *ldr, void *buf, size_t len);
int llext_fs_seek(struct llext_loader *ldr, size_t pos);
void llext_fs_finalize(struct llext_loader *ldr);
/** @endcond */

/**
 * @brief Initializer for an llext_fs_loader structure
 *
 * @param _filename Absolute path to the extension file.
 */
#define LLEXT_FS_LOADER(_filename)                                                                 \
	{                                                                                          \
		.loader =                                                                          \
			{                                                                          \
				.prepare = llext_fs_prepare,                                       \
				.read = llext_fs_read,                                             \
				.seek = llext_fs_seek,                                             \
				.peek = NULL,                                                      \
				.finalize = llext_fs_finalize,                                     \
			},                                                                         \
		.is_open = false,                                                                  \
		.name = (_filename),                                                               \
	}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_FS_LOADER_H */
