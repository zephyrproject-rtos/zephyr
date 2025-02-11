/*
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_MEMORY_REGIONS_H_
#define ZEPHYR_SOC_INTEL_ADSP_MEMORY_REGIONS_H_

/* Define amount of regions other than core heaps that virtual memory will be split to
 * currently includes shared heap and oma region and one regions set to 0 as for table
 * iterator end value.
 */
#define VIRTUAL_REGION_COUNT 3

#define CORE_HEAP_SIZE 0x100000
#define SHARED_HEAP_SIZE 0x100000
#define OPPORTUNISTIC_REGION_SIZE 0x100000

/* size of TLB table */
#define TLB_SIZE DT_REG_SIZE_BY_IDX(DT_INST(0, intel_adsp_mtl_tlb), 0)

/* Attribiutes for memory regions */
#define MEM_REG_ATTR_CORE_HEAP 1U
#define MEM_REG_ATTR_SHARED_HEAP 2U
#define MEM_REG_ATTR_OPPORTUNISTIC_MEMORY 4U

#endif /* ZEPHYR_SOC_INTEL_ADSP_MEMORY_REGIONS_H_ */
