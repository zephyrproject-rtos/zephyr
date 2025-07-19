/*
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_MEMORY_REGIONS_H_
#define ZEPHYR_SOC_INTEL_ADSP_MEMORY_REGIONS_H_

/**
 * Add a virtual memory region to the table
 *
 * @param virtual_address an address of a new region
 * @param region_size a size of a new region
 * @param attr an attribute of a new region, may not be unique
 *
 * @return 0 success
 * @return -ENOMEM if there are no empty slots for virtual memory regions
 * @return -EINVAL if virtual memory region overlaps with existing ones or exceeds memory size
 */
int adsp_add_virtual_memory_region(uintptr_t region_address, uint32_t region_size, uint32_t attr);

/* size of TLB table */
#define TLB_SIZE DT_REG_SIZE_BY_IDX(DT_INST(0, intel_adsp_mtl_tlb), 0)


#endif /* ZEPHYR_SOC_INTEL_ADSP_MEMORY_REGIONS_H_ */
