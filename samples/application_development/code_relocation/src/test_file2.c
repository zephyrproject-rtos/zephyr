/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

void function_in_sram(s32_t value)
{
	printk("Address of function_in_sram %p\n", &function_in_sram);
	printk("Hello World! %s\n", CONFIG_BOARD);
}
