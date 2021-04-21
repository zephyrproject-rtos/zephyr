/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

uint32_t var_sram2_data = 10U;
uint32_t var_sram2_bss;
K_SEM_DEFINE(test, 0, 1);
const uint32_t var_sram2_rodata = 100U;

__in_section(custom_section, static, var) uint32_t var_custom_data = 1U;

extern void function_in_sram(int32_t value);
void function_in_custom_section(void);
void function_in_sram2(void)
{
	/* Print values from sram2 */
	printk("Address of function_in_sram2 %p\n", &function_in_sram2);
	printk("Address of var_sram2_data %p\n", &var_sram2_data);
	printk("Address of k_sem_give %p\n", &k_sem_give);
	printk("Address of var_sram2_rodata %p\n", &var_sram2_rodata);
	printk("Address of var_sram2_bss %p\n\n", &var_sram2_bss);

	/* Print values from sram */
	function_in_sram(var_sram2_data);

	/* Print values which were placed using attributes */
	printk("Address of custom_section, func placed using attributes %p\n",
	       &function_in_custom_section);
	printk("Address of custom_section data placed using attributes %p\n\n",
	       &var_custom_data);

	k_sem_give(&test);
}

__in_section(custom_section, static, fun) void function_in_custom_section(void)
{
	return;

}
