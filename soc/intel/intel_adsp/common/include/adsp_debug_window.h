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
 *
 * The descriptor layout is:
 *
 *	--------------------
 *	| Desc0  - slot0   |
 *	--------------------
 *	| Desc1  - slot1   |
 *	--------------------
 *	| Desc2  - slot2   |
 *	--------------------
 *	|       ...        |
 *	--------------------
 *	| Desc13  - slot13 |
 *	--------------------
 *	| Desc14  - slot14 |
 *	--------------------
 *
 * Additional descriptor to describe the function of the partial slot at page0:
 *
 *	--------------------------
 *	| Desc15  - page0 + 1024 |
 *	--------------------------
 */

#define ADSP_DW_PAGE_SIZE		0x1000
#define ADSP_DW_SLOT_SIZE		ADSP_DW_PAGE_SIZE
#define ADSP_DW_SLOT_COUNT		15
#define ADSP_DW_PAGE0_SLOT_OFFSET	1024
#define ADSP_DW_DESC_COUNT		(ADSP_DW_SLOT_COUNT + 1)

/* debug log slot types */
#define ADSP_DW_SLOT_UNUSED		0x00000000
#define ADSP_DW_SLOT_CRITICAL_LOG	0x54524300
#define ADSP_DW_SLOT_DEBUG_LOG		0x474f4c00 /* byte 0: core ID */
#define ADSP_DW_SLOT_GDB_STUB		0x42444700
#define ADSP_DW_SLOT_TELEMETRY		0x4c455400
#define ADSP_DW_SLOT_TRACE		0x54524143
#define ADSP_DW_SLOT_SHELL		0x73686c6c
#define ADSP_DW_SLOT_DEBUG_STREAM	0x53523134
#define ADSP_DW_SLOT_BROKEN		0x44414544

 /* for debug and critical types */
#define ADSP_DW_SLOT_CORE_MASK		GENMASK(7, 0)
#define ADSP_DW_SLOT_TYPE_MASK		GENMASK(31, 8)

struct adsp_dw_desc {
	uint32_t resource_id;
	uint32_t type;
	uint32_t vma;
} __packed;

#ifdef CONFIG_INTEL_ADSP_DEBUG_SLOT_MANAGER
/**
 * @brief Request a free debug slot for a function described by desc
 *
 * if a slot for the same function has been already allocated, the existing slot will be returned
 *
 * @param dw_desc	Description of the slot to be requested for
 * @param slot_size	Optional pointer to receive back the size of the assigned slot, if not
 *			provided then the partial slot from window 0 cannot be requested as caller
 *			must be aware and be able to handle the different size of the slot
 *
 * @return		Pointer to the start of the slot or NULL in case of an error. When NULL is
 *			returned, the @slot_size value is undefined.
 */
void *adsp_dw_request_slot(struct adsp_dw_desc *dw_desc, size_t *slot_size);

/**
 * @brief Forcibly overtake a slot
 *
 * @param slot_index	Index of the slot to take.
 *			Requesting ADSP_DW_FULL_SLOTS will return the partial slot
 * @param dw_desc	Description of the slot to be requested for
 * @param slot_size	Optional pointer to receive back the size of the assigned slot, if not
 *			provided then the partial slot from window 0 cannot be requested as caller
 *			must be aware and be able to handle the different size of the slot
 *
 * @return		Pointer to the start of the slot or NULL in case of an error. When NULL is
 *			returned, the @slot_size value is undefined.
 */
void *adsp_dw_seize_slot(uint32_t slot_index, struct adsp_dw_desc *dw_desc,
				  size_t *slot_size);

/**(
 * @brief Release a slot allocated for type
 *
 * @param type		Slot type to be released
 *
 * @note		Only a single slot can be allocated at the same time for a type
 */
void adsp_dw_release_slot(uint32_t type);
#else /* CONFIG_INTEL_ADSP_DEBUG_SLOT_MANAGER */

/* debug window slots usage, mutually exclusive options can reuse slots */
#define ADSP_DW_SLOT_NUM_SHELL		0
#define ADSP_DW_SLOT_NUM_MTRACE		0
#define ADSP_DW_SLOT_NUM_TRACE		1
#define ADSP_DW_SLOT_NUM_TELEMETRY	1
/* this uses remaining space in the first page after descriptors */
#define ADSP_DW_SLOT_NUM_GDB		(ADSP_DW_DESC_COUNT - 1)

struct adsp_debug_window {
	struct adsp_dw_desc descs[ADSP_DW_DESC_COUNT];
	uint8_t reserved[ADSP_DW_PAGE0_SLOT_OFFSET -
			 ADSP_DW_DESC_COUNT * sizeof(struct adsp_dw_desc)];
	uint8_t partial_page0[ADSP_DW_SLOT_SIZE - ADSP_DW_PAGE0_SLOT_OFFSET];
	uint8_t slots[ADSP_DW_SLOT_COUNT][ADSP_DW_SLOT_SIZE];
} __packed;

#define WIN2_MBASE DT_REG_ADDR(DT_PHANDLE(DT_NODELABEL(mem_window2), memory))

#define ADSP_DW ((volatile struct adsp_debug_window *) \
		 (sys_cache_uncached_ptr_get((__sparse_force void __sparse_cache *) \
				     (WIN2_MBASE + WIN2_OFFSET))))

#endif /* CONFIG_INTEL_ADSP_DEBUG_SLOT_MANAGER */

#endif
