/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LLEXT_PRIV_H_
#define ZEPHYR_SUBSYS_LLEXT_PRIV_H_

#include <zephyr/kernel.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/llext_internal.h>
#include <zephyr/sys/slist.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

/*
 * Macro to determine if section / region is in instruction memory
 * Will need to be updated if any non-ARC boards using Harvard architecture is added
 */
#if CONFIG_HARVARD && CONFIG_ARC
#define IN_NODE(inst, compat, base_addr, alloc)                                                    \
	(((uintptr_t)(base_addr) >= DT_REG_ADDR(DT_INST(inst, compat)) &&                          \
	  (uintptr_t)(base_addr + alloc) <=                                                        \
		  DT_REG_ADDR(DT_INST(inst, compat)) + DT_REG_SIZE(DT_INST(inst, compat)))) ||
#define INSTR_FETCHABLE(base_addr, alloc)                                                          \
	DT_COMPAT_FOREACH_STATUS_OKAY_VARGS(arc_iccm, IN_NODE, base_addr, alloc) false
#elif CONFIG_HARVARD && !CONFIG_ARC
/* Unknown if section / region is in instruction memory; warn or compensate */
#define INSTR_FETCHABLE(base_addr, alloc) false
#else /* all non-Harvard architectures */
#define INSTR_FETCHABLE(base_addr, alloc) true
#endif

/*
 * Global extension list
 */

extern sys_slist_t llext_list;
extern struct k_mutex llext_lock;

/*
 * Memory management (llext_mem.c)
 */

int llext_copy_strings(struct llext_loader *ldr, struct llext *ext,
		       const struct llext_load_param *ldr_parm);
int llext_copy_regions(struct llext_loader *ldr, struct llext *ext,
		       const struct llext_load_param *ldr_parm);
void llext_free_regions(struct llext *ext);
void llext_adjust_mmu_permissions(struct llext *ext);

#ifdef CONFIG_HARVARD
extern struct k_heap llext_instr_heap;
extern struct k_heap llext_data_heap;
#else
extern struct k_heap llext_heap;
#define llext_instr_heap llext_heap
#define llext_data_heap  llext_heap
#endif

static inline bool llext_heap_is_inited(void)
{
#ifdef CONFIG_LLEXT_HEAP_DYNAMIC
	extern bool llext_heap_inited;

	return llext_heap_inited;
#else
	return true;
#endif
}

static inline void *llext_alloc_data(size_t bytes)
{
	if (!llext_heap_is_inited()) {
		return NULL;
	}

	/* Used for LLEXT metadata */
	return k_heap_alloc(&llext_data_heap, bytes, K_NO_WAIT);
}

static inline void *llext_aligned_alloc_data(size_t align, size_t bytes)
{
	if (!llext_heap_is_inited()) {
		return NULL;
	}

	/* Used for LLEXT metadata OR non-executable section */
	return k_heap_aligned_alloc(&llext_data_heap, align, bytes, K_NO_WAIT);
}

static inline void llext_free(void *ptr)
{
	if (!llext_heap_is_inited()) {
		return;
	}

	k_heap_free(&llext_data_heap, ptr);
}

static inline void *llext_aligned_alloc_instr(size_t align, size_t bytes)
{
	if (!llext_heap_is_inited()) {
		return NULL;
	}

	return k_heap_aligned_alloc(&llext_instr_heap, align, bytes, K_NO_WAIT);
}

static inline void llext_free_instr(void *ptr)
{
	if (!llext_heap_is_inited()) {
		return;
	}

	k_heap_free(&llext_instr_heap, ptr);
}

/*
 * ELF parsing (llext_load.c)
 */

int do_llext_load(struct llext_loader *ldr, struct llext *ext,
		  const struct llext_load_param *ldr_parm);

/*
 * Relocation (llext_link.c)
 */

int llext_link(struct llext_loader *ldr, struct llext *ext,
	       const struct llext_load_param *ldr_parm);
ssize_t llext_file_offset(struct llext_loader *ldr, uintptr_t offset);
void llext_dependency_remove_all(struct llext *ext);

#endif /* ZEPHYR_SUBSYS_LLEXT_PRIV_H_ */
