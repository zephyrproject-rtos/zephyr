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
 * @brief Storage type for the ELF data to be loaded.
 *
 * This enum defines the storage type of the ELF data that will be loaded. The
 * storage type determines which memory optimizations can be tried by the LLEXT
 * subsystem during the load process.
 *
 * @note Even with the most permissive option, LLEXT might still have to copy
 * ELF data into a separate memory region to comply with other restrictions,
 * such as hardware memory protection and/or alignment rules.
 * Sections such as BSS that have no space in the file will also be allocated
 * in the LLEXT heap.
 */
enum llext_storage_type {
	/**
	 * ELF data is only available during llext_load(); even if the loader
	 * supports directly accessing the memory via llext_peek(), the buffer
	 * contents will be discarded afterwards.
	 * LLEXT will allocate copies of all required data into its heap.
	 */
	LLEXT_STORAGE_TEMPORARY,
	/**
	 * ELF data is stored in a *read-only* buffer that is guaranteed to be
	 * always accessible for as long as the extension is loaded. LLEXT may
	 * directly reuse constant data from the buffer, but may still allocate
	 * copies if relocations need to be applied.
	 */
	LLEXT_STORAGE_PERSISTENT,
	/**
	 * ELF data is stored in a *writable* memory buffer that is guaranteed
	 * to be always accessible for as long as the extension is loaded.
	 * LLEXT may freely modify and reuse data in the buffer; so, after the
	 * extension is unloaded, the contents should be re-initialized before
	 * a subsequent llext_load() call.
	 */
	LLEXT_STORAGE_WRITABLE,
};

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

	/**
	 * @brief Storage type of the underlying data accessed by this loader.
	 */
	enum llext_storage_type storage;

	/** @cond ignore */
	elf_ehdr_t hdr;
	elf_shdr_t sects[LLEXT_MEM_COUNT];
	struct llext_elf_sect_map *sect_map;
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
