/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header for agregating all defines for mm
 *
 */
#ifndef ZEPHYR_DRIVERS_SYSTEM_MM_DRV_INTEL_MTL_
#define ZEPHYR_DRIVERS_SYSTEM_MM_DRV_INTEL_MTL_

#define DT_DRV_COMPAT intel_adsp_mtl_tlb

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/check.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mm/system_mm.h>
#include <zephyr/sys/mem_blocks.h>

#include <soc.h>
#include <adsp_memory.h>
#include <adsp_memory_regions.h>

#include "mm_drv_common.h"

#define TLB_BASE (mm_reg_t)DT_REG_ADDR(DT_NODELABEL(tlb))

/*
 * Number of significant bits in the page index (defines the size of
 * the table)
 */
#define TLB_PADDR_SIZE DT_INST_PROP(0, paddr_size)
#define TLB_EXEC_BIT   BIT(DT_INST_PROP(0, exec_bit_idx))
#define TLB_WRITE_BIT  BIT(DT_INST_PROP(0, write_bit_idx))

#define TLB_ENTRY_NUM (1 << TLB_PADDR_SIZE)
#define TLB_PADDR_MASK ((1 << TLB_PADDR_SIZE) - 1)
#define TLB_ENABLE_BIT BIT(TLB_PADDR_SIZE)

/* This is used to translate from TLB entry back to physical address. */
/* base address of TLB table */
#define TLB_PHYS_BASE  \
	(((L2_SRAM_BASE / CONFIG_MM_DRV_PAGE_SIZE) & ~TLB_PADDR_MASK) * CONFIG_MM_DRV_PAGE_SIZE)
#define HPSRAM_SEGMENTS(hpsram_ebb_quantity) \
	((ROUND_DOWN((hpsram_ebb_quantity) + 31u, 32u) / 32u) - 1u)

#define L2_SRAM_PAGES_NUM			(L2_SRAM_SIZE / CONFIG_MM_DRV_PAGE_SIZE)
#define MAX_EBB_BANKS_IN_SEGMENT	32
#define SRAM_BANK_SIZE				(128 * 1024)
#define L2_SRAM_BANK_NUM			(L2_SRAM_SIZE / SRAM_BANK_SIZE)

/**
 * Calculate TLB entry based on physical address.
 *
 * @param pa Page-aligned virutal address.
 * @return TLB entry value.
 */
static inline uint16_t pa_to_tlb_entry(uintptr_t pa)
{
	return (((pa) / CONFIG_MM_DRV_PAGE_SIZE) & TLB_PADDR_MASK);
}

/**
 * Calculate physical address based on TLB entry.
 *
 * @param tlb_entry TLB entry value.
 * @return physcial address pointer.
 */
static inline uintptr_t tlb_entry_to_pa(uint16_t tlb_entry)
{
	return ((((tlb_entry) & TLB_PADDR_MASK) *
		CONFIG_MM_DRV_PAGE_SIZE) + TLB_PHYS_BASE);
}

#endif /* ZEPHYR_DRIVERS_SYSTEM_MM_DRV_INTEL_MTL_ */
