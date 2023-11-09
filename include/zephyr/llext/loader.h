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
 * @brief Loader context for llext
 * @defgroup llext_loader Loader context for llext
 * @ingroup llext
 * @{
 */

/**
 * @brief Enum of sections for lookup tables
 */
enum llext_section {
	LLEXT_SECT_TEXT,
	LLEXT_SECT_DATA,
	LLEXT_SECT_RODATA,
	LLEXT_SECT_BSS,

	LLEXT_SECT_REL_TEXT,
	LLEXT_SECT_REL_DATA,
	LLEXT_SECT_REL_RODATA,
	LLEXT_SECT_REL_BSS,

	LLEXT_SECT_EXPORT,

	LLEXT_SECT_SYMTAB,
	LLEXT_SECT_STRTAB,
	LLEXT_SECT_SHSTRTAB,

	LLEXT_SECT_COUNT,
};

enum llext_mem;

/**
 * @brief Linkable loadable extension loader context
 */
struct llext_loader {
	/**
	 * @brief Read (copy) from the loader
	 *
	 * Copies len bytes into buf from the current position of the
	 * loader.
	 *
	 * @param[in] ldr Loader
	 * @param[in] out Output location
	 * @param[in] len Length to copy into the output location
	 *
	 * @retval 0 Success
	 * @retval -errno Error reading (any errno)
	 */
	int (*read)(struct llext_loader *ldr, void *out, size_t len);

	/**
	 * @brief Seek to a new absolute location
	 *
	 * Changes the location of the loader position to a new absolute
	 * given position.
	 *
	 * @param[in] ldr Loader
	 * @param[in] pos Position in stream to move loader
	 *
	 * @retval 0 Success
	 * @retval -errno Error reading (any errno)
	 */
	int (*seek)(struct llext_loader *ldr, size_t pos);

	/**
	 * @brief Peek at an absolute location
	 *
	 * Return a pointer to the buffer at specified offset.
	 *
	 * @param[in] ldr Loader
	 * @param[in] pos Position to obtain a pointer to
	 *
	 * @retval pointer into the buffer
	 */
	void *(*peek)(struct llext_loader *ldr, size_t pos);

	/** @cond ignore */
	elf_ehdr_t hdr;
	elf_shdr_t sects[LLEXT_SECT_COUNT];
	enum llext_mem *sect_map;
	uint32_t sect_cnt;
	/** @endcond */
};

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

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_LOADER_H */
