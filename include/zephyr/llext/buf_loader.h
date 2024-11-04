/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_BUF_LOADER_H
#define ZEPHYR_LLEXT_BUF_LOADER_H

#include <zephyr/llext/loader.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief LLEXT buffer loader implementation.
 *
 * @addtogroup llext_loader_apis
 * @{
 */

/**
 * @brief Implementation of @ref llext_loader that reads from a memory buffer.
 */
struct llext_buf_loader {
	/** Extension loader */
	struct llext_loader loader;

	/** @cond ignore */
	const uint8_t *buf;
	size_t len;
	size_t pos;
	/** @endcond */
};

/** @cond ignore */
int llext_buf_read(struct llext_loader *ldr, void *buf, size_t len);
int llext_buf_seek(struct llext_loader *ldr, size_t pos);
void *llext_buf_peek(struct llext_loader *ldr, size_t pos);

#define Z_LLEXT_BUF_LOADER(_buf, _buf_len, _storage)                                               \
	{                                                                                          \
		.loader =                                                                          \
			{                                                                          \
				.prepare = NULL,                                                   \
				.read = llext_buf_read,                                            \
				.seek = llext_buf_seek,                                            \
				.peek = llext_buf_peek,                                            \
				.finalize = NULL,                                                  \
				.storage = _storage,                                               \
			},                                                                         \
		.buf = (_buf),                                                                     \
		.len = (_buf_len),                                                                 \
		.pos = 0,                                                                          \
	}
/** @endcond */

/**
 * @brief Initializer for an llext_buf_loader structure
 *
 * The storage type for the provided buffer depends on the value of the
 * @kconfig{CONFIG_LLEXT_STORAGE_WRITABLE} option: if it is defined, the
 * buffer is assumed to be writable; otherwise it is assumed to be persistent.
 *
 * Consider using one of the alternative macros instead.
 *
 * @see LLEXT_TEMPORARY_BUF_LOADER
 * @see LLEXT_PERSISTENT_BUF_LOADER
 * @see LLEXT_WRITABLE_BUF_LOADER
 *
 * @param _buf Buffer containing the ELF binary
 * @param _buf_len Buffer length in bytes
 */
#define LLEXT_BUF_LOADER(_buf, _buf_len) \
	Z_LLEXT_BUF_LOADER(_buf, _buf_len,                                                         \
			   IS_ENABLED(CONFIG_LLEXT_STORAGE_WRITABLE) ?                             \
				LLEXT_STORAGE_WRITABLE : LLEXT_STORAGE_PERSISTENT)

/* @brief Initialize an llext_buf_loader structure for a temporary buffer
 *
 * ELF data from the specified buffer can only be used during llext_load().
 * The LLEXT subsystem will copy all necessary data to internal buffers at load
 * time.
 *
 * @param _buf Buffer containing the ELF binary
 * @param _buf_len Buffer length in bytes
 */
#define LLEXT_TEMPORARY_BUF_LOADER(_buf, _buf_len) \
	Z_LLEXT_BUF_LOADER(_buf, _buf_len, LLEXT_STORAGE_TEMPORARY)

/**
 * @brief Initialize an llext_buf_loader structure for a persistent, read-only buffer
 *
 * ELF data from the specified buffer is guaranteed to be accessible for as
 * long as the extension is loaded. The LLEXT subsystem may directly access the
 * ELF data, as long as no modification is required during loading.
 *
 * @param _buf Buffer containing the ELF binary
 * @param _buf_len Buffer length in bytes
 */
#define LLEXT_PERSISTENT_BUF_LOADER(_buf, _buf_len) \
	Z_LLEXT_BUF_LOADER(_buf, _buf_len, LLEXT_STORAGE_PERSISTENT)

/**
 * @brief Initialize an llext_buf_loader structure for a persistent, writable buffer
 *
 * ELF data from the specified buffer is guaranteed to be accessible for as
 * long as the extension is loaded. The LLEXT subsystem may directly access and
 * modify the ELF data.
 *
 * @param _buf Buffer containing the ELF binary
 * @param _buf_len Buffer length in bytes
 */
#define LLEXT_WRITABLE_BUF_LOADER(_buf, _buf_len) \
	Z_LLEXT_BUF_LOADER(_buf, _buf_len, LLEXT_STORAGE_WRITABLE)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_BUF_LOADER_H */
