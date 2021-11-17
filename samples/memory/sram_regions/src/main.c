/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <linker/linker-defs.h>
#include <linker/devicetree_sram.h>

uint32_t var_in_region0 __attribute((__section__("region0"))) = 0xaabbccdd;
uint32_t var_in_region1 __attribute((__section__("region1"))) = 0xddccbbaa;

uint32_t buf_in_nocache[0xA] __nocache_sram;

void main(void)
{
	printk("Address of var_in_region0: %p\n", &var_in_region0);
	printk("Address of var_in_region1: %p\n", &var_in_region1);

	printk("Address of buf_in_nocache: %p\n", buf_in_nocache);
}
