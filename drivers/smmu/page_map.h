/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PAGE_MAP_H__
#define __PAGE_MAP_H__

#include <zephyr/kernel.h>

#include "smmu_pte.h"

struct page_map {
	struct k_mutex mux;
	mem_addr_t paddr;
	pd_entry_t *base_xlat_table;
	uint16_t base_xlat_level;
	uint16_t va_size;
};

uint64_t* page_table_alloc_empty();

#endif /* end of include guard: __PAGE_MAP_H__ */
