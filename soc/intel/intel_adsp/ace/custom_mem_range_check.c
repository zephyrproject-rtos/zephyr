/*
 * Copyright (c) 2023, 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/xtensa/arch.h>
#include <zephyr/arch/xtensa/cache.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/cache.h>
#include <zephyr/devicetree.h>

bool sys_mm_is_phys_addr_in_range(uintptr_t phys)
{
	bool valid;
	uintptr_t cached = (uintptr_t)sys_cache_cached_ptr_get((void *)phys);

	valid = ((phys >= DT_CHOSEN_SRAM_ADDR) &&
		 (phys < (DT_CHOSEN_SRAM_ADDR + DT_CHOSEN_SRAM_SIZE)));

	valid |= ((cached >= DT_CHOSEN_SRAM_ADDR) &&
		  (cached < (DT_CHOSEN_SRAM_ADDR + DT_CHOSEN_SRAM_SIZE)));

	return valid;
}

bool sys_mm_is_virt_addr_in_range(void *virt)
{
	bool valid;
	uintptr_t addr = (uintptr_t)virt;

	uintptr_t cached = (uintptr_t)sys_cache_cached_ptr_get(virt);

	valid = ((addr >= CONFIG_KERNEL_VM_BASE) &&
		 (addr < (CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_SIZE)));

	valid |= ((cached >= CONFIG_KERNEL_VM_BASE) &&
		  (cached < (CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_SIZE)));

	return valid;
}
