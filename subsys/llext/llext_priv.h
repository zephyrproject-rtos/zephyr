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

#ifdef CONFIG_LLEXT_HEAP_K_HEAP
#include "llext_kheap.h"
#elif defined(CONFIG_LLEXT_HEAP_MEMBLK)
#include "llext_memblk.h"
#else
#error "No LLEXT heap implementation selected; see CONFIG_LLEXT_HEAP_MANAGEMENT"
#endif

/*
 * Macro to determine if section / region is in instruction memory
 * Will need to be updated if any non-ARC boards using Harvard architecture is added
 */
#if CONFIG_HARVARD && CONFIG_ARC
#define IN_NODE(inst, compat, base_addr, alloc)                                                    \
	(((uintptr_t)(base_addr) >= DT_REG_ADDR(DT_INST(inst, compat)) &&                          \
	  ((uintptr_t)(base_addr) + (uintptr_t)(alloc)) <=                                         \
		  DT_REG_ADDR(DT_INST(inst, compat)) + DT_REG_SIZE(DT_INST(inst, compat)))) ||
#define INSTR_FETCHABLE(base_addr, alloc)                                                          \
	(DT_COMPAT_FOREACH_STATUS_OKAY_VARGS(arc_iccm, IN_NODE, base_addr, alloc) false)
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
