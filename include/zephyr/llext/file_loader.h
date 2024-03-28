/*
 * Copyright (c) 2024 Schneider Electric
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_FILE_LOADER_H
#define ZEPHYR_LLEXT_FILE_LOADER_H

#include <zephyr/llext/loader.h>
#include <zephyr/fs/fs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LLEXT file loader
 * @defgroup llext_file_loader Linkable loadable extensions file loader
 * @ingroup llext
 * @{
 */

/**
 * @brief An extension loader from a provided file containing an ELF
 */
struct llext_file_loader {
	/** Extension loader */
	struct llext_loader loader;

	/** @cond ignore */
	struct fs_file_t *fd;
	/** @endcond */
};

/** @cond ignore */
int llext_file_read(struct llext_loader *ldr, void *buf, size_t len);
int llext_file_seek(struct llext_loader *ldr, size_t pos);
/** @endcond */

/**
 * @brief Initialize an extension buf loader
 *
 * @param _fd File descriptor containing an ELF binary
 * @param _buf_len Buffer length in bytes
 */
#define LLEXT_FILE_LOADER(_fd)		\
	{						\
		.loader = {				\
			.read = llext_file_read,		\
			.seek = llext_file_seek		\
		},					\
		.fd = (_fd),				\
	}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_FILE_LOADER_H */
