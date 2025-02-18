/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_INTERNAL_H
#define ZEPHYR_LLEXT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/llext/llext.h>

/**
 * @file
 * @brief Private header for linkable loadable extensions
 */

/** @cond ignore */

static inline const char *llext_string(const struct llext_loader *ldr, const struct llext *ext,
				       enum llext_mem mem_idx, unsigned int idx)
{
	return (char *)ext->mem[mem_idx] + idx;
}

struct llext_elf_sect_map {
	enum llext_mem mem_idx;
	size_t offset;
};

const void *llext_loaded_sect_ptr(struct llext_loader *ldr, struct llext *ext, unsigned int sh_ndx);

static inline uintptr_t llext_text_start(const struct llext *ext)
{
	return (uintptr_t)ext->mem[LLEXT_MEM_TEXT];
}

/**
 * Determine address of a symbol.
 */
int llext_lookup_symbol(struct llext_loader *ldr, struct llext *ext, uintptr_t *link_addr,
			const elf_rela_t *rel, const elf_sym_t *sym, const char *name,
			const elf_shdr_t *shdr);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_INTERNAL_H */
