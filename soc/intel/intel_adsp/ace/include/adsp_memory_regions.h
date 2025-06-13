/*
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_MEMORY_REGIONS_H_
#define ZEPHYR_SOC_INTEL_ADSP_MEMORY_REGIONS_H_

/* size of TLB table */
#define TLB_SIZE DT_REG_SIZE_BY_IDX(DT_INST(0, intel_adsp_mtl_tlb), 0)

/* Attribiutes for memory regions */
#if CONFIG_MM_DRV_INTEL_ADSP_TLB_USE_PER_CORE_VIRTUAL_MEMORY_REGIONS
#define VIRTUAL_REGION_COUNT_PER_CORE CONFIG_MP_MAX_NUM_CPUS
#define MEM_REG_ATTR_CORE_HEAP 1U
#else
#define VIRTUAL_REGION_COUNT_PER_CORE 0
#endif

#if CONFIG_MM_DRV_INTEL_ADSP_TLB_USE_VIRTUAL_MEMORY_SHARED_REGION
#define VIRTUAL_REGION_COUNT_SHARED 1
#define MEM_REG_ATTR_SHARED_HEAP 2U
#else
#define VIRTUAL_REGION_COUNT_SHARED 0
#endif

#if CONFIG_MM_DRV_INTEL_ADSP_TLB_USE_VIRTUAL_MEMORY_OPPORTUNISTIC_REGION
#define VIRTUAL_REGION_COUNT_OPPORTUNISTIC 1
#define MEM_REG_ATTR_OPPORTUNISTIC_MEMORY 4U
#else
#define VIRTUAL_REGION_COUNT_OPPORTUNISTIC 0
#endif

/* Define amount of regions other than core heaps that virtual memory will be split to
 * currently includes shared heap and oma region and one regions set to 0 as for table
 * iterator end value.
 */
#define VIRTUAL_REGION_COUNT (VIRTUAL_REGION_COUNT_PER_CORE +		\
			      VIRTUAL_REGION_COUNT_SHARED +		\
			      VIRTUAL_REGION_COUNT_OPPORTUNISTIC + 1)


#endif /* ZEPHYR_SOC_INTEL_ADSP_MEMORY_REGIONS_H_ */
