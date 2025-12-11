/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

uint32_t var_sram_data = 10U;

void function_in_sram(void)
{
	printk("Address of %s %p\n", __func__, &function_in_sram);
	printk("Address of var_sram_data %p (%d)\n", &var_sram_data, var_sram_data);
}
