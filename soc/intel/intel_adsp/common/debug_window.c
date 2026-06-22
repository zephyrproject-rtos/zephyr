/* Copyright (c) 2025 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <adsp_memory.h>
#include <adsp_debug_window.h>
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(debug_window, CONFIG_SOC_LOG_LEVEL);

struct adsp_debug_window {
	struct adsp_dw_desc descs[ADSP_DW_DESC_COUNT];
	uint8_t reserved[ADSP_DW_PAGE0_SLOT_OFFSET -
			 ADSP_DW_DESC_COUNT * sizeof(struct adsp_dw_desc)];
	uint8_t partial_page0[ADSP_DW_SLOT_SIZE - ADSP_DW_PAGE0_SLOT_OFFSET];
	uint8_t slots[ADSP_DW_SLOT_COUNT][ADSP_DW_SLOT_SIZE];
} __packed;

#define WIN2_MBASE		DT_REG_ADDR(DT_PHANDLE(DT_NODELABEL(mem_window2), memory))
#define WIN2_SLOTS		((HP_SRAM_WIN2_SIZE / ADSP_DW_PAGE_SIZE) - 1)
#define ADSP_DW_FULL_SLOTS	MIN(WIN2_SLOTS, ADSP_DW_SLOT_COUNT)

#define ADSP_DW ((struct adsp_debug_window *) \
		  (sys_cache_uncached_ptr_get((__sparse_force void __sparse_cache *) \
				     (WIN2_MBASE + WIN2_OFFSET))))

static int adsp_dw_find_unused_slot(bool prefer_partial)
{
	int i;

	if (prefer_partial) {
		if (ADSP_DW->descs[ADSP_DW_SLOT_COUNT].type == ADSP_DW_SLOT_UNUSED) {
			return ADSP_DW_SLOT_COUNT;
		}
	}

	for (i = 0; i < ADSP_DW_FULL_SLOTS; i++) {
		if (ADSP_DW->descs[i].type == ADSP_DW_SLOT_UNUSED) {
			return i;
		}
	}

	if (!prefer_partial) {
		if (ADSP_DW->descs[ADSP_DW_SLOT_COUNT].type == ADSP_DW_SLOT_UNUSED) {
			return ADSP_DW_SLOT_COUNT;
		}
	}

	return -ENOENT;
}

static int adsp_dw_find_slot_by_type(uint32_t type)
{
	int i;

	for (i = 0; i < ADSP_DW_FULL_SLOTS; i++) {
		if (ADSP_DW->descs[i].type == type) {
			return i;
		}
	}

	if (ADSP_DW->descs[ADSP_DW_SLOT_COUNT].type == type) {
		return ADSP_DW_SLOT_COUNT;
	}

	return -ENOENT;
}

void *adsp_dw_request_slot(struct adsp_dw_desc *dw_desc, size_t *slot_size)
{
	int slot_idx;

	if (!dw_desc->type) {
		return NULL;
	}

	/* Check if a slot has been allocated for this type */
	slot_idx = adsp_dw_find_slot_by_type(dw_desc->type);
	if (slot_idx >= 0) {
		if (slot_size) {
			*slot_size = ADSP_DW_SLOT_SIZE;
		}

		if (slot_idx == ADSP_DW_SLOT_COUNT) {
			if (slot_size) {
				*slot_size -= ADSP_DW_PAGE0_SLOT_OFFSET;
			}
			return ADSP_DW->partial_page0;
		}

		return ADSP_DW->slots[slot_idx];
	}

	/* Look for an empty slot */
	slot_idx = adsp_dw_find_unused_slot(!!slot_size);
	if (slot_idx < 0) {
		LOG_INF("No available slot for %#x", dw_desc->type);
		return NULL;
	}

	ADSP_DW->descs[slot_idx].resource_id = dw_desc->resource_id;
	ADSP_DW->descs[slot_idx].type = dw_desc->type;
	ADSP_DW->descs[slot_idx].vma = dw_desc->vma;

	if (slot_size) {
		*slot_size = ADSP_DW_SLOT_SIZE;
	}

	if (slot_idx == ADSP_DW_SLOT_COUNT) {
		if (slot_size) {
			*slot_size -= ADSP_DW_PAGE0_SLOT_OFFSET;
		}

		LOG_DBG("Allocating partial page0 to be used for %#x", dw_desc->type);

		return ADSP_DW->partial_page0;
	}

	LOG_DBG("Allocating debug slot %d to be used for %#x", slot_idx, dw_desc->type);

	return ADSP_DW->slots[slot_idx];
}

void *adsp_dw_seize_slot(uint32_t slot_index, struct adsp_dw_desc *dw_desc,
				  size_t *slot_size)
{
	if ((slot_index >= ADSP_DW_FULL_SLOTS && slot_index != ADSP_DW_SLOT_COUNT) ||
	    !dw_desc->type) {
		return NULL;
	}

	if (slot_index < ADSP_DW_FULL_SLOTS) {
		if (ADSP_DW->descs[slot_index].type != ADSP_DW_SLOT_UNUSED) {
			LOG_INF("Overtaking debug slot %d, used by %#x for %#x", slot_index,
				ADSP_DW->descs[slot_index].type, dw_desc->type);
		}

		if (slot_size) {
			*slot_size = ADSP_DW_SLOT_SIZE;
		}

		ADSP_DW->descs[slot_index].resource_id = dw_desc->resource_id;
		ADSP_DW->descs[slot_index].type = dw_desc->type;
		ADSP_DW->descs[slot_index].vma = dw_desc->vma;

		return ADSP_DW->slots[slot_index];
	}

	/* The caller is not prepared to handle partial debug slot */
	if (!slot_size) {
		return NULL;
	}

	if (ADSP_DW->descs[slot_index].type != ADSP_DW_SLOT_UNUSED) {
		LOG_INF("Overtaking partial page0, used by %#x for %#x",
			ADSP_DW->descs[slot_index].type, dw_desc->type);
	}

	*slot_size = ADSP_DW_SLOT_SIZE - ADSP_DW_PAGE0_SLOT_OFFSET;

	ADSP_DW->descs[slot_index].resource_id = dw_desc->resource_id;
	ADSP_DW->descs[slot_index].type = dw_desc->type;
	ADSP_DW->descs[slot_index].vma = dw_desc->vma;

	return ADSP_DW->partial_page0;
}

void adsp_dw_release_slot(uint32_t type)
{
	int slot_idx;

	slot_idx = adsp_dw_find_slot_by_type(type);
	if (slot_idx < 0) {
		return;
	}

	if (slot_idx == ADSP_DW_SLOT_COUNT) {
		LOG_DBG("Releasing partial page0 used by %#x", type);
	} else {
		LOG_DBG("Releasing debug slot %d used by %#x", slot_idx, type);
	}

	ADSP_DW->descs[slot_idx].resource_id = 0;
	ADSP_DW->descs[slot_idx].type = ADSP_DW_SLOT_UNUSED;
	ADSP_DW->descs[slot_idx].vma = 0;
}
