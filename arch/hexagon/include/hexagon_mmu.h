/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_MMU_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_MMU_H_

#include <stddef.h>
#include <stdint.h>
#include <hexagon_vm.h>

/* Linear list every installed map ends with, see mmu.c. */
extern struct hexagon_linear_entry hexagon_fixed_tail[];

/* Map installed while no user thread's map is. */
extern struct hexagon_linear_entry hexagon_kernel_map[];

/* Round [*addr, *addr + *size) outward to whole 4KB pages. */
void hexagon_align_region_to_page(uintptr_t *addr, size_t *size);

/*
 * Map the page-aligned [addr, addr + size) 1:1 with at most max_entries
 * linear entries written to entries[]. Return the number used.
 */
uint32_t hexagon_decompose_region(struct hexagon_linear_entry *entries, uint32_t max_entries,
				  uintptr_t addr, size_t size, uint32_t xwru, uint32_t cache_attr);

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_MMU_H_ */
