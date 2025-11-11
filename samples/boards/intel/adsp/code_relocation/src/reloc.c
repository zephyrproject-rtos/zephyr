/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

static uint32_t maybe_bss;

void reloc(void)
{
	printk("Relocated code! %s location %p\n", __func__, reloc);
	printk("maybe_bss location: %p maybe_bss value: %u\n", &maybe_bss,
	       maybe_bss);
}
