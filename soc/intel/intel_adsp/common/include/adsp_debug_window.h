/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ZEPHYR_SOC_INTEL_ADSP_DEBUG_WINDOW
#define _ZEPHYR_SOC_INTEL_ADSP_DEBUG_WINDOW

#include <mem_window.h>
#include <zephyr/debug/sparse.h>
#include <zephyr/cache.h>

/*
 * SRAM window for debug info (window 2) is organized in slots,
 * described in the descriptors available on page 0.
 *
 *	------------------------
 *	| Page0  - descriptors |
 *	------------------------
 *	| Page1  - slot0       |
 *	------------------------
 *	| Page2  - slot1       |
 *	------------------------
 *	|         ...          |
 *	------------------------
 *	| Page14  - slot13     |
 *	------------------------
 *	| Page15  - slot14     |
 *	------------------------
 *
 * The slot size == page size
 *
 * The first page contains descriptors for the remaining slots.
 * The overall number of slots can vary based on platform.
 *
 * The slot descriptor is:
 * u32 res_id;
 * u32 type;
 * u32 vma;
 */

#define ADSP_DW_PAGE_SIZE		0x1000
#define ADSP_DW_SLOT_SIZE		ADSP_DW_PAGE_SIZE
#define ADSP_DW_SLOT_COUNT		15

/* debug log slot types */
#define ADSP_DW_SLOT_UNUSED		0x00000000
#define ADSP_DW_SLOT_CRITICAL_LOG	0x54524300
#define ADSP_DW_SLOT_DEBUG_LOG		0x474f4c00 /* byte 0: core ID */
#define ADSP_DW_SLOT_GDB_STUB		0x42444700
#define ADSP_DW_SLOT_TELEMETRY		0x4c455400
#define ADSP_DW_SLOT_BROKEN		0x44414544

 /* for debug and critical types */
#define ADSP_DW_SLOT_CORE_MASK		GENMASK(7, 0)
#define ADSP_DW_SLOT_TYPE_MASK		GENMASK(31, 8)

struct adsp_dw_desc {
	uint32_t resource_id;
	uint32_t type;
	uint32_t vma;
} __packed;

struct adsp_debug_window {
	struct adsp_dw_desc descs[ADSP_DW_SLOT_COUNT];
	uint8_t reserved[ADSP_DW_SLOT_SIZE - ADSP_DW_SLOT_COUNT * sizeof(struct adsp_dw_desc)];
	uint8_t slots[ADSP_DW_SLOT_COUNT][ADSP_DW_SLOT_SIZE];
} __packed;

#define WIN2_MBASE DT_REG_ADDR(DT_PHANDLE(DT_NODELABEL(mem_window2), memory))

#define ADSP_DW ((volatile struct adsp_debug_window *) \
		 (sys_cache_uncached_ptr_get((__sparse_force void __sparse_cache *) \
				     (WIN2_MBASE + WIN2_OFFSET))))

#endif
