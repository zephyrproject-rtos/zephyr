/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

uint32_t var_ext_sram_data = 10U;

void function_in_ext_flash(void)
{
	printk("Address of %s %p\n", __func__, &function_in_ext_flash);
	printk("Address of var_ext_sram_data %p (%d)\n", &var_ext_sram_data, var_ext_sram_data);
}
