/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <ztest.h>

uint32_t var_file3_sram_data = 10U;
uint32_t var_file3_sram2_bss;

ZTEST(code_relocation, test_function_in_split_multiple)
{
	extern uint32_t __data_start;
	extern uint32_t __data_end;
	extern uint32_t __sram2_bss_start;
	extern uint32_t __sram2_bss_end;

	printk("Address of var_file3_sram_data %p\n", &var_file3_sram_data);
	printk("Address of var_file3_sram2_bss %p\n\n", &var_file3_sram2_bss);

	zassert_between_inclusive((uint32_t)&var_file3_sram_data,
		(uint32_t)&__data_start,
		(uint32_t)&__data_end,
		"var_file3_sram_data not in sram_data region");
	zassert_between_inclusive((uint32_t)&var_file3_sram2_bss,
		(uint32_t)&__sram2_bss_start,
		(uint32_t)&__sram2_bss_end,
		"var_file3_sram2_bss not in sram2_bss region");
}
