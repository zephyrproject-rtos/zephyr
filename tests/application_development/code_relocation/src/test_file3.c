/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

__in_section(data, sram, var) uint32_t var_file3_sram_data = 10U;
__in_section(bss, sram2, var) uint32_t var_file3_sram2_bss;

ZTEST(code_relocation, test_function_in_split_multiple)
{
	extern uintptr_t __data_start;
	extern uintptr_t __data_end;
	extern uintptr_t __sram2_bss_reloc_start;
	extern uintptr_t __sram2_bss_reloc_end;

	printk("Address of var_file3_sram_data %p\n", &var_file3_sram_data);
	printk("Address of var_file3_sram2_bss %p\n\n", &var_file3_sram2_bss);

	zassert_between_inclusive((uintptr_t)&var_file3_sram_data,
		(uintptr_t)&__data_start,
		(uintptr_t)&__data_end,
		"var_file3_sram_data not in sram_data region");
	zassert_between_inclusive((uintptr_t)&var_file3_sram2_bss,
		(uintptr_t)&__sram2_bss_reloc_start,
		(uintptr_t)&__sram2_bss_reloc_end,
		"var_file3_sram2_bss not in sram2_bss region");
}
