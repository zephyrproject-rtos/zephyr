/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

#include "test_lib.h"

/*
 * These values will typically be placed in the appropriate sections, but may be moved around
 * by the compiler; for instance var_sram2_data might end up in .rodata if the compiler can prove
 * that it's never modified. To prevent that, we explicitly specify sections.
 */
__in_section(data, sram2, var) uint32_t var_sram2_data = 10U;
__in_section(bss, sram2, var) uint32_t var_sram2_bss;
K_SEM_DEFINE(test, 0, 1);
__in_section(rodata, sram2, var) const uint32_t var_sram2_rodata = 100U;

__in_section(custom_section, static, var) uint32_t var_custom_data = 1U;

extern void function_in_sram(int32_t value);
extern void function_not_relocated(int32_t value);
void function_in_custom_section(void);

#define HAS_SRAM2_DATA_SECTION (CONFIG_ARM)

ZTEST(code_relocation, test_function_in_sram2)
{
	extern uintptr_t __ram_text_reloc_start;
	extern uintptr_t __ram_text_reloc_end;
	extern uintptr_t __sram2_text_reloc_start;
	extern uintptr_t __sram2_text_reloc_end;
	extern uintptr_t __sram2_data_reloc_start;
	extern uintptr_t __sram2_data_reloc_end;
	extern uintptr_t __sram2_bss_reloc_start;
	extern uintptr_t __sram2_bss_reloc_end;
	extern uintptr_t __sram2_rodata_reloc_start;
	extern uintptr_t __sram2_rodata_reloc_end;
	extern uintptr_t __custom_section_start;
	extern uintptr_t __custom_section_end;

	/* Print values from sram2 */
	printk("Address of var_sram2_data %p\n", &var_sram2_data);
	printk("Address of k_sem_give %p\n", &k_sem_give);
	printk("Address of var_sram2_rodata %p\n", &var_sram2_rodata);
	printk("Address of var_sram2_bss %p\n\n", &var_sram2_bss);

	zassert_between_inclusive((uintptr_t)&var_sram2_data,
		(uintptr_t)&__sram2_data_reloc_start,
		(uintptr_t)&__sram2_data_reloc_end,
		"var_sram2_data not in sram2 region");
	zassert_between_inclusive((uintptr_t)&k_sem_give,
		(uintptr_t)&__sram2_text_reloc_start,
		(uintptr_t)&__sram2_text_reloc_end,
		"k_sem_give not in sram_text region");
	zassert_between_inclusive((uintptr_t)&var_sram2_rodata,
		(uintptr_t)&__sram2_rodata_reloc_start,
		(uintptr_t)&__sram2_rodata_reloc_end,
		"var_sram2_rodata not in sram2_rodata region");
	zassert_between_inclusive((uintptr_t)&var_sram2_bss,
		(uintptr_t)&__sram2_bss_reloc_start,
		(uintptr_t)&__sram2_bss_reloc_end,
		"var_sram2_bss not in sram2_bss region");

	/* Print values from sram */
	printk("Address of function_in_sram %p\n", &function_in_sram);
	zassert_between_inclusive((uintptr_t)&function_in_sram,
		(uintptr_t)&__ram_text_reloc_start,
		(uintptr_t)&__ram_text_reloc_end,
		"function_in_sram is not in ram region");
	function_in_sram(var_sram2_data);

	/* Print values from non-relocated function */
	printk("Address of function_not_relocated %p\n", &function_not_relocated);
	zassert_between_inclusive((uintptr_t)&function_not_relocated,
		(uintptr_t)&__text_region_start,
		(uintptr_t)&__text_region_end,
		"function_not_relocated is not in flash region");
	function_not_relocated(var_sram2_data);
	/* Call library function */
	relocated_library();

	/* Print values which were placed using attributes */
	printk("Address of custom_section, func placed using attributes %p\n",
	       &function_in_custom_section);
	printk("Address of custom_section data placed using attributes %p\n\n",
	       &var_custom_data);
	zassert_between_inclusive((uintptr_t)&function_in_custom_section,
		(uintptr_t)&__custom_section_start,
		(uintptr_t)&__custom_section_end,
		"function_in_custom_section not in custom_section region");
	zassert_between_inclusive((uintptr_t)&var_custom_data,
		(uintptr_t)&__custom_section_start,
		(uintptr_t)&__custom_section_end,
		"var_custom_data not in custom_section region");

	k_sem_give(&test);
}

__in_section(custom_section, static, fun) void function_in_custom_section(void)
{
}
