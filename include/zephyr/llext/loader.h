/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_LOADER_H
#define ZEPHYR_LLEXT_LOADER_H

#include <zephyr/llext/elf.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief LLEXT ELF loader context types.
 *
 * The following types are used to define the context of the ELF loader
 * used by the \ref llext subsystem.
 *
 * @defgroup llext_loader_apis ELF loader context
 * @ingroup llext_apis
 * @{
 */

#include <zephyr/llext/llext.h>

/** @cond ignore */
struct llext_elf_sect_map; /* defined in llext_priv.h */
/** @endcond */

/**
 * @brief Linkable loadable extension loader context
 *
 * This object is used to access the ELF file data and cache its contents
 * while an extension is being loaded by the LLEXT subsystem. Once the
 * extension is loaded, this object is no longer needed.
 */
struct llext_loader {
	/**
	 * @brief Optional function to prepare the loader for loading extension.
	 *
	 * @param[in] ldr Loader
	 *
	 * @returns 0 on success, or a negative error.
	 */
	int (*prepare)(struct llext_loader *ldr);

	/**
	 * @brief Function to read (copy) from the loader
	 *
	 * Copies len bytes into buf from the current position of the
	 * loader.
	 *
	 * @param[in] ldr Loader
	 * @param[in] out Output location
	 * @param[in] len Length to copy into the output location
	 *
	 * @returns 0 on success, or a negative error code.
	 */
	int (*read)(struct llext_loader *ldr, void *out, size_t len);

	/**
	 * @brief Function to seek to a new absolute location in the stream.
	 *
	 * Changes the location of the loader position to a new absolute
	 * given position.
	 *
	 * @param[in] ldr Loader
	 * @param[in] pos Position in stream to move loader
	 *
	 * @returns 0 on success, or a negative error code.
	 */
	int (*seek)(struct llext_loader *ldr, size_t pos);

	/**
	 * @brief Optional function to peek at an absolute location in the ELF.
	 *
	 * Return a pointer to the buffer at specified offset.
	 *
	 * @param[in] ldr Loader
	 * @param[in] pos Position to obtain a pointer to
	 *
	 * @returns a pointer into the buffer or `NULL` if not supported
	 */
	void *(*peek)(struct llext_loader *ldr, size_t pos);

	/**
	 * @brief Optional function to clean after the extension has been loaded or error occurred.
	 *
	 * @param[in] ldr Loader
	 */
	void (*finalize)(struct llext_loader *ldr);

	/** @cond ignore */
	elf_ehdr_t hdr;
	elf_shdr_t sects[LLEXT_MEM_COUNT];
	elf_shdr_t *sect_hdrs;
	bool sect_hdrs_on_heap;
	struct llext_elf_sect_map *sect_map;
	uint32_t sect_cnt;
	/** @endcond */
};

/** @cond ignore */
static inline int llext_prepare(struct llext_loader *l)
{
	if (l->prepare) {
		return l->prepare(l);
	}

	return 0;
}

static inline int llext_read(struct llext_loader *l, void *buf, size_t len)
{
	return l->read(l, buf, len);
}

static inline int llext_seek(struct llext_loader *l, size_t pos)
{
	return l->seek(l, pos);
}

static inline void *llext_peek(struct llext_loader *l, size_t pos)
{
	if (l->peek) {
		return l->peek(l, pos);
	}

	return NULL;
}

static inline void llext_finalize(struct llext_loader *l)
{
	if (l->finalize) {
		l->finalize(l);
	}
}
/* @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_LOADER_H */
