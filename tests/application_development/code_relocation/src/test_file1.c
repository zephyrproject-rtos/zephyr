/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <ztest.h>

uint32_t var_sram2_data = 10U;
uint32_t var_sram2_bss;
K_SEM_DEFINE(test, 0, 1);
const uint32_t var_sram2_rodata = 100U;

__in_section(custom_section, static, var) uint32_t var_custom_data = 1U;

extern void function_in_sram(int32_t value);
void function_in_custom_section(void);

ZTEST(code_relocation, test_function_in_sram2)
{
	extern uint32_t __sram2_text_start;
	extern uint32_t __sram2_text_end;
	extern uint32_t __sram2_data_start;
	extern uint32_t __sram2_data_end;
	extern uint32_t __sram2_bss_start;
	extern uint32_t __sram2_bss_end;
	extern uint32_t __sram2_rodata_start;
	extern uint32_t __sram2_rodata_end;
	extern uint32_t __custom_section_start;
	extern uint32_t __custom_section_end;

	/* Print values from sram2 */
	printk("Address of var_sram2_data %p\n", &var_sram2_data);
	printk("Address of k_sem_give %p\n", &k_sem_give);
	printk("Address of var_sram2_rodata %p\n", &var_sram2_rodata);
	printk("Address of var_sram2_bss %p\n\n", &var_sram2_bss);

	zassert_between_inclusive((uint32_t)&var_sram2_data,
		(uint32_t)&__sram2_data_start,
		(uint32_t)&__sram2_data_end,
		"var_sram2_data not in sram2 region");
	zassert_between_inclusive((uint32_t)&k_sem_give,
		(uint32_t)&__sram2_text_start,
		(uint32_t)&__sram2_text_end,
		"k_sem_give not in sram_text region");
	zassert_between_inclusive((uint32_t)&var_sram2_rodata,
		(uint32_t)&__sram2_rodata_start,
		(uint32_t)&__sram2_rodata_end,
		"var_sram2_rodata not in sram2_rodata region");
	zassert_between_inclusive((uint32_t)&var_sram2_bss,
		(uint32_t)&__sram2_bss_start,
		(uint32_t)&__sram2_bss_end,
		"var_sram2_bss not in sram2_bss region");

	/* Print values from sram */
	function_in_sram(var_sram2_data);

	/* Print values which were placed using attributes */
	printk("Address of custom_section, func placed using attributes %p\n",
	       &function_in_custom_section);
	printk("Address of custom_section data placed using attributes %p\n\n",
	       &var_custom_data);
	zassert_between_inclusive((uint32_t)&function_in_custom_section,
		(uint32_t)&__custom_section_start,
		(uint32_t)&__custom_section_end,
		"function_in_custom_section not in custom_section region");
	zassert_between_inclusive((uint32_t)&var_custom_data,
		(uint32_t)&__custom_section_start,
		(uint32_t)&__custom_section_end,
		"var_custom_data not in custom_section region");

	k_sem_give(&test);
}

__in_section(custom_section, static, fun) void function_in_custom_section(void)
{
}
