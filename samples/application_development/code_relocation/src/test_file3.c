/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>

uint32_t var_file3_sram_data = 10U;
uint32_t var_file3_sram2_bss;

void function_in_split_multiple(void)
{
	printk("Address of function_in_split_multiple %p\n",
	       &function_in_split_multiple);
	printk("Address of var_file3_sram_data %p\n", &var_file3_sram_data);
	printk("Address of var_file3_sram2_bss %p\n\n", &var_file3_sram2_bss);
}
