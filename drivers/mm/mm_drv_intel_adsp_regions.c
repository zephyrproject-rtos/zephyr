/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver handling memory regions related
 * functions
 */

#include "mm_drv_intel_adsp.h"

struct sys_mm_drv_region
virtual_memory_regions[CONFIG_MP_MAX_NUM_CPUS + VIRTUAL_REGION_COUNT] = { {0} };

const struct sys_mm_drv_region *sys_mm_drv_query_memory_regions(void)
{
	return (const struct sys_mm_drv_region *) virtual_memory_regions;
}

static inline void append_region(void *address, uint32_t mem_size,
	uint32_t attributes, uint32_t position)
{
	virtual_memory_regions[position].addr = address;
	virtual_memory_regions[position].size = mem_size;
	virtual_memory_regions[position].attr = attributes;
}

int calculate_memory_regions(uintptr_t static_alloc_end_ptr)
{
	int total_size = 0;
	int pos = 0;

#if CONFIG_MM_DRV_INTEL_ADSP_TLB_USE_PER_CORE_VIRTUAL_MEMORY_REGIONS
	for (int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		append_region((void *)(static_alloc_end_ptr + total_size),
				CONFIG_MM_DRV_INTEL_ADSP_TLB_PER_CORE_VIRTUAL_MEMORY_REGIONS_SIZE,
				MEM_REG_ATTR_CORE_HEAP, pos++);
		total_size += CONFIG_MM_DRV_INTEL_ADSP_TLB_PER_CORE_VIRTUAL_MEMORY_REGIONS_SIZE;
	}
#endif

#if CONFIG_MM_DRV_INTEL_ADSP_TLB_USE_VIRTUAL_MEMORY_SHARED_REGION
	append_region((void *)(static_alloc_end_ptr + total_size),
			CONFIG_MM_DRV_INTEL_ADSP_TLB_VIRTUAL_MEMORY_SHARED_REGION_SIZE,
			MEM_REG_ATTR_SHARED_HEAP, pos++);
	total_size += CONFIG_MM_DRV_INTEL_ADSP_TLB_VIRTUAL_MEMORY_SHARED_REGION_SIZE;
#endif

#if CONFIG_MM_DRV_INTEL_ADSP_TLB_USE_VIRTUAL_MEMORY_OPPORTUNISTIC_REGION
	append_region((void *)(static_alloc_end_ptr + total_size),
			CONFIG_MM_DRV_INTEL_ADSP_TLB_VIRTUAL_MEMORY_OPPORTUNISTIC_REGION_SIZE,
			MEM_REG_ATTR_OPPORTUNISTIC_MEMORY, pos++);
	total_size += CONFIG_MM_DRV_INTEL_ADSP_TLB_VIRTUAL_MEMORY_OPPORTUNISTIC_REGION_SIZE;
#endif
	/* Appending last region as 0 so iterators know where table is over
	 * check is for size = 0;
	 */
	append_region(NULL, 0, 0, pos++);

	if (total_size > L2_VIRTUAL_SRAM_SIZE) {
		return -EINVAL;
	}

	return 0;
}
