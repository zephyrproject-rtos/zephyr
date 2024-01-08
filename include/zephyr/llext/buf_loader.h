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
 * @brief LLEXT buffer loader
 * @defgroup llext_buf_loader Linkable loadable extensions buffer loader
 * @ingroup llext
 * @{
 */

/**
 * @brief An extension loader from a provided buffer containing an ELF
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
/** @endcond */

/**
 * @brief Initialize an extension buf loader
 *
 * @param _buf Buffer containing an ELF binary
 * @param _buf_len Buffer length in bytes
 */
#define LLEXT_BUF_LOADER(_buf, _buf_len)		\
	{						\
		.loader = {				\
			.read = llext_buf_read,		\
			.seek = llext_buf_seek,		\
			.peek = llext_buf_peek,		\
		},					\
		.buf = (_buf),				\
		.len = (_buf_len),			\
		.pos = 0				\
	}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_BUF_LOADER_H */
