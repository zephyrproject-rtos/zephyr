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


/* virtual memory regions table, last item is an end marker */
struct sys_mm_drv_region
virtual_memory_regions[CONFIG_MM_DRV_INTEL_VIRTUAL_REGION_COUNT+1] = { {0} };

const struct sys_mm_drv_region *sys_mm_drv_query_memory_regions(void)
{
	return (const struct sys_mm_drv_region *) virtual_memory_regions;
}

int adsp_add_virtual_memory_region(uintptr_t region_address, uint32_t region_size, uint32_t attr)
{
	struct sys_mm_drv_region *region;
	uint32_t pos = 0;
	uintptr_t new_region_end = region_address + region_size;

	/* check if the region fits to virtual memory */
	if (region_address < L2_VIRTUAL_SRAM_BASE ||
	    new_region_end > L2_VIRTUAL_SRAM_BASE + L2_VIRTUAL_SRAM_SIZE) {
		return -EINVAL;
	}

	/* find an empty slot, verify if the region is not overlapping */
	SYS_MM_DRV_MEMORY_REGION_FOREACH(virtual_memory_regions, region) {
		uintptr_t region_start = (uintptr_t)region->addr;
		uintptr_t region_end = region_start + region->size;

		/* check region overlapping */
		if (region_address < region_end && new_region_end > region_start) {
			return -EINVAL;
		}
		pos++;
	}

	/* SYS_MM_DRV_MEMORY_REGION_FOREACH exits when an empty slot is found */
	if (pos == CONFIG_MM_DRV_INTEL_VIRTUAL_REGION_COUNT) {
		/* no more free slots */
		return -ENOMEM;
	}

	/* add new region */
	virtual_memory_regions[pos].addr = (void *)region_address;
	virtual_memory_regions[pos].size = region_size;
	virtual_memory_regions[pos].attr = attr;

	return 0;
}
