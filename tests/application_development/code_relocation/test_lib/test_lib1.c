/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

__in_section(data, sram2, var) uint32_t var_lib1_sram2_data = 10U;
__in_section(bss, sram2, var) uint32_t var_lib1_sram2_bss;

/* Helper function, declared in test_lib2.c */
extern void relocated_helper(void);

void relocated_library(void)
{
	extern uintptr_t __sram2_text_reloc_start;
	extern uintptr_t __sram2_text_reloc_end;
	extern uintptr_t __sram2_data_reloc_start;
	extern uintptr_t __sram2_data_reloc_end;
	extern uintptr_t __sram2_bss_reloc_start;
	extern uintptr_t __sram2_bss_reloc_end;

	printk("Address of var_lib1_sram2_data %p\n", &var_lib1_sram2_data);
	printk("Address of var_lib1_sram2_bss %p\n", &var_lib1_sram2_bss);
	printk("Address of relocated_lib_helper %p\n\n", &relocated_helper);

	zassert_between_inclusive((uintptr_t)&var_lib1_sram2_data,
		(uintptr_t)&__sram2_data_reloc_start,
		(uintptr_t)&__sram2_data_reloc_end,
		"var_lib1_sram2_data not in sram2_data region");
	zassert_between_inclusive((uintptr_t)&var_lib1_sram2_bss,
		(uintptr_t)&__sram2_bss_reloc_start,
		(uintptr_t)&__sram2_bss_reloc_end,
		"var_lib1_sram2_bss not in sram2_bss region");
	zassert_between_inclusive((uintptr_t)&relocated_helper,
		(uintptr_t)&__sram2_text_reloc_start,
		(uintptr_t)&__sram2_text_reloc_end,
		"relocated_helper not in sram2_text region");
	relocated_helper();
}
