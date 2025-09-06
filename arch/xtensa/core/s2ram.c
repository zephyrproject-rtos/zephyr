/*
 * Copyright (c) 2025 Meta Platforms, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa/hal-certified.h>
#include <xtensa/hal-core-state.h>

#include <zephyr/arch/common/pm_s2ram.h>
#include <xtensa_s2ram.h>

struct xtensa_s2ram_save_area p2sram_saved_area;

extern uint8_t p2sram_entry;
int arch_pm_s2ram_suspend(pm_s2ram_system_off_fn_t system_off)
{
	p2sram_saved_area.magic = S2RAM_MAGIC;
	p2sram_saved_area.system_off = (uintptr_t)system_off;
	xthal_core_save(0, &p2sram_saved_area.core_state, (pm_s2ram_system_off_fn_t)&p2sram_entry);
	p2sram_saved_area.magic = 0;
	return 0;
}
