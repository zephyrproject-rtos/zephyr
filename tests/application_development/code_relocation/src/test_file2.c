/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <ztest.h>

void function_in_sram(int32_t value)
{
	char src[8] = "data\n";
	char dst[8];

	printk("Hello World! %s\n", CONFIG_BOARD);
	memcpy(dst, src, 8);
	printk("Address of memcpy %p\n", &memcpy);
	zassert_mem_equal(src, dst, 8, "memcpy compare error");
}
