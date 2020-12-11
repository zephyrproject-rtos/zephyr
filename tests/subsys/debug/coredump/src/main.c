/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

void func_3(uint32_t *addr)
{
	/* For null pointer reference */
	*addr = 0;
}

void func_2(uint32_t *addr)
{
	func_3(addr);
}

void func_1(uint32_t *addr)
{
	func_2(addr);
}

void main(void)
{
	printk("Coredump: %s\n", CONFIG_BOARD);

	func_1(0);
}
