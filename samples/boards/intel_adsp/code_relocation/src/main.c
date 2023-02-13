/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

extern void reloc(void);

void main(void)
{
	printk("%s location: %p\n", __func__, main);
	printk("Calling relocated code\n");
	reloc();
}
