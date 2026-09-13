/*
 * Copyright 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_management/clock_helpers.h>


int clock_management_mux_validate_parent(uint32_t parent_cnt, uint32_t parent)
{
	if (parent >= parent_cnt) {
		return -EINVAL;
	}

	return parent;
}

int clock_management_mux_get_parent(uintptr_t reg, uint8_t mask_width, uint8_t mask_offset,
				    uint32_t parent_cnt)
{
	const uint32_t mux_mask = GENMASK((mask_width + mask_offset - 1), mask_offset);
	const uint32_t sel = ((sys_read32(reg) & mux_mask) >> mask_offset);

	return clock_management_mux_validate_parent(parent_cnt, sel);
}

int clock_management_mux_set_parent(uintptr_t reg, uint8_t mask_width, uint8_t mask_offset,
				    uint32_t parent_cnt, uint32_t parent)
{
	const uint32_t mux_mask = GENMASK((mask_width + mask_offset - 1), mask_offset);
	const uint32_t mux_val = FIELD_PREP(mux_mask, parent);

	if (clock_management_mux_validate_parent(parent_cnt, parent) < 0) {
		return -EINVAL;
	}

	sys_write32((sys_read32(reg) & ~mux_mask) | mux_val, reg);

	return 0;
}
