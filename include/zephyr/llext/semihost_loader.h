/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_SEMIHOST_LOADER_H
#define ZEPHYR_LLEXT_SEMIHOST_LOADER_H

#include <zephyr/llext/loader.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief LLEXT semihost loader implementation.
 *
 * @addtogroup llext_loader_apis
 * @{
 */

/**
 * @brief Implementation of @ref llext_loader that reads through semihosting.
 */
struct llext_semihost_loader {
	/** Extension loader */
	struct llext_loader loader;

	/** @cond ignore */
	const char *filename;
	int fd;
	size_t total_bytes;
	/** @endcond */
};

/** @cond ignore */
int llext_semihost_prepare(struct llext_loader *ldr);
int llext_semihost_read(struct llext_loader *ldr, void *buf, size_t len);
int llext_semihost_seek(struct llext_loader *ldr, size_t pos);
void llext_semihost_finalize(struct llext_loader *ldr);
/** @endcond */

/**
 * @brief Initializer for an llext_semihost_loader structure
 *
 * @param _filename Path to the extension file.
 */
#define LLEXT_SEMIHOST_LOADER(_filename)                                                           \
	{                                                                                          \
		.loader =                                                                          \
			{                                                                          \
				.prepare = llext_semihost_prepare,                                 \
				.read = llext_semihost_read,                                       \
				.seek = llext_semihost_seek,                                       \
				.peek = NULL,                                                      \
				.finalize = llext_semihost_finalize,                               \
				.storage = LLEXT_STORAGE_TEMPORARY,                                \
			},                                                                         \
		.filename = (_filename),                                                           \
		.fd = -1,                                                                          \
	}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_SEMIHOST_LOADER_H */
