/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/arch/x86/memmap.h>
#include <zephyr/linker/linker-defs.h>
#include <kernel_arch_data.h>

struct x86_memmap_exclusion x86_memmap_exclusions[] = {
#ifdef CONFIG_X86_64
	{ "locore", _locore_start, _locore_end },
#endif
#ifdef CONFIG_XIP
	{ "rom", __rom_region_start, __rom_region_end },
#endif
	{ "ram", _image_ram_start, _image_ram_end },
#ifdef CONFIG_USERSPACE
	{ "app_smem", _app_smem_start, _app_smem_end },
#endif
#ifdef CONFIG_COVERAGE_GCOV
	{ "gcov", __gcov_bss_start, __gcov_bss_end },
#endif
};

int x86_nr_memmap_exclusions = sizeof(x86_memmap_exclusions) /
			       sizeof(struct x86_memmap_exclusion);

/*
 * The default map symbols are weak so that an application
 * can override with a hardcoded manual map if desired.
 */

__weak enum x86_memmap_source x86_memmap_source = X86_MEMMAP_SOURCE_DEFAULT;

__weak struct x86_memmap_entry x86_memmap[CONFIG_X86_MEMMAP_ENTRIES] = {
	{
		DT_REG_ADDR(DT_CHOSEN(zephyr_sram)),
		DT_REG_SIZE(DT_CHOSEN(zephyr_sram)),
		X86_MEMMAP_ENTRY_RAM
	}
};
