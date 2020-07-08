/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <x86_mmu.h>

void main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);
	z_x86_dump_page_tables(&z_x86_kernel_ptables);
}
