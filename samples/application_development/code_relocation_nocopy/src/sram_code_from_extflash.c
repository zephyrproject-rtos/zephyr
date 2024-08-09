/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

void function_in_sram_from_extflash(void)
{
	printk("Address of %s %p\n", __func__, &function_in_sram_from_extflash);
}
