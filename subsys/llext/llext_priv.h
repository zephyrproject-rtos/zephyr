/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LLEXT_PRIV_H_
#define ZEPHYR_SUBSYS_LLEXT_PRIV_H_

#include <zephyr/kernel.h>
#include <zephyr/llext/llext.h>

/*
 * Memory management (llext_mem.c)
 */

int llext_copy_strings(struct llext_loader *ldr, struct llext *ext);
int llext_copy_sections(struct llext_loader *ldr, struct llext *ext);
void llext_free_sections(struct llext *ext);

static inline void *llext_alloc(size_t bytes)
{
	extern struct k_heap llext_heap;

	return k_heap_alloc(&llext_heap, bytes, K_NO_WAIT);
}

static inline void *llext_aligned_alloc(size_t align, size_t bytes)
{
	extern struct k_heap llext_heap;

	return k_heap_aligned_alloc(&llext_heap, align, bytes, K_NO_WAIT);
}

static inline void llext_free(void *ptr)
{
	extern struct k_heap llext_heap;

	k_heap_free(&llext_heap, ptr);
}

/*
 * ELF parsing (llext_load.c)
 */

int do_llext_load(struct llext_loader *ldr, struct llext *ext,
		  struct llext_load_param *ldr_parm);

elf_shdr_t *llext_section_by_name(struct llext_loader *ldr, const char *search_name);

static inline const char *llext_string(struct llext_loader *ldr, struct llext *ext,
				       enum llext_mem mem_idx, unsigned int idx)
{
	return (char *)ext->mem[mem_idx] + idx;
}

/*
 * Relocation (llext_link.c)
 */

int llext_link(struct llext_loader *ldr, struct llext *ext, bool do_local);

#endif /* ZEPHYR_SUBSYS_LLEXT_PRIV_H_ */
