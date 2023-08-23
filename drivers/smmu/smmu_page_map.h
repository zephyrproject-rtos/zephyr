/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SMMU_PAGE_MAP_H__
#define __SMMU_PAGE_MAP_H__

#include "page_map.h"

/*
 * Calculate the initial translation table level from CONFIG_ARM64_VA_BITS
 * For a 4 KB page size:
 *
 * (va_bits <= 21)	 - unsupported
 * (22 <= va_bits <= 30) - base level 2
 * (31 <= va_bits <= 39) - base level 1
 * (40 <= va_bits <= 48) - base level 0
 */
#define SMMU_GET_BASE_XLAT_LEVEL(va_bits)	\
	 ((va_bits > (SMMU_L0_S)) ? 0U		\
	: (va_bits > (SMMU_L1_S)) ? 1U : 2U)

int page_map_init(struct page_map *pmap, uint16_t va_size);
int page_map_release(struct page_map *pmap);
int page_map_smmu_add(struct page_map *pmap, mem_addr_t va, mem_addr_t pa, int32_t extra_attrs);
int page_map_smmu_remove(struct page_map *pmap, mem_addr_t va);

#endif /* end of include guard: __SMMU_PAGE_MAP_H__ */
