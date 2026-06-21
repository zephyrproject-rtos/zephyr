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

#ifdef CONFIG_LLEXT_HEAP_K_HEAP
#include "llext_kheap.h"
#elif defined(CONFIG_LLEXT_HEAP_MEMBLK)
#include "llext_memblk.h"
#else
#error "No LLEXT heap implementation selected; see CONFIG_LLEXT_HEAP_MANAGEMENT"
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
