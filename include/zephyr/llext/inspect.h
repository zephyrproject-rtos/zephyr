/*
 * Copyright (c) 2025 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_INSPECT_H
#define ZEPHYR_LLEXT_INSPECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/loader.h>
#include <zephyr/llext/llext_internal.h>

/**
 * @file
 * @brief LLEXT ELF inspection routines.
 *
 * This file contains routines to inspect the contents of an ELF file. It is
 * intended to be used by applications that need advanced access to the ELF
 * file structures of a loaded extension.
 *
 * @defgroup llext_inspect_apis ELF inspection APIs
 * @ingroup llext_apis
 * @{
 */

/**
 * @brief Get information about a memory region for the specified extension.
 *
 * Retrieve information about a region (merged group of similar sections) in
 * the extension. Any output parameter can be NULL if that information is not
 * needed.
 *
 * @param[in] ldr Loader
 * @param[in] ext Extension
 * @param[in] region Region to get information about
 * @param[out] hdr Variable storing the pointer to the region header
 * @param[out] addr Variable storing the region load address
 * @param[out] size Variable storing the region size
 *
 * @return 0 on success, -EINVAL if the region is invalid
 */
static inline int llext_get_region_info(const struct llext_loader *ldr,
					const struct llext *ext,
					enum llext_mem region,
					const elf_shdr_t **hdr,
					const void **addr, size_t *size)
{
	if ((unsigned int)region >= LLEXT_MEM_COUNT) {
		return -EINVAL;
	}

	if (hdr) {
		*hdr = &ldr->sects[region];
	}
	if (addr) {
		*addr = ext->mem[region];
	}
	if (size) {
		*size = ext->mem_size[region];
	}

	return 0;
}

/**
 * @brief Get the index of a section with the specified name.
 *
 * Requires the @ref llext_load_param.keep_section_info flag to be set at
 * extension load time.
 *
 * @param[in] ldr Loader
 * @param[in] ext Extension
 * @param[in] section_name Name of the section to look for
 *
 * @return Section index on success, -ENOENT if the section was not found,
 *         -ENOTSUP if section data is not available.
 */
int llext_section_shndx(const struct llext_loader *ldr, const struct llext *ext,
			    const char *section_name);

/**
 * @brief Get information about a section for the specified extension.
 *
 * Retrieve information about an ELF sections in the extension. Any output
 * parameter can be @c NULL if that information is not needed.
 *
 * Requires the @ref llext_load_param.keep_section_info flag to be set at
 * extension load time.
 *
 * @param[in] ldr Loader
 * @param[in] ext Extension
 * @param[in] shndx Section index
 * @param[out] hdr Variable storing the pointer to the section header
 * @param[out] region Variable storing the region the section belongs to
 * @param[out] offset Variable storing the offset of the section in the region
 *
 * @return 0 on success, -EINVAL if the section index is invalid,
 *         -ENOTSUP if section data is not available.
 */
static inline int llext_get_section_info(const struct llext_loader *ldr,
					 const struct llext *ext,
					 unsigned int shndx,
					 const elf_shdr_t **hdr,
					 enum llext_mem *region,
					 size_t *offset)
{
	if (shndx < 0 || shndx >= ext->sect_cnt) {
		return -EINVAL;
	}
	if (!ldr->sect_map) {
		return -ENOTSUP;
	}

	if (hdr) {
		*hdr = &ext->sect_hdrs[shndx];
	}
	if (region) {
		*region = ldr->sect_map[shndx].mem_idx;
	}
	if (offset) {
		*offset = ldr->sect_map[shndx].offset;
	}

	return 0;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_INSPECT_H */
