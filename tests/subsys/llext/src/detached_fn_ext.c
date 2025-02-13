/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest_assert.h>

Z_GENERIC_SECTION(.detach) void detached_entry(void)
{
	static int data_cnt = -3;
	static unsigned int bss_cnt;

	printk("bss %u @ %p\n", bss_cnt++, &bss_cnt);
	printk("data %d @ %p\n", data_cnt++, &data_cnt);
	zassert_true(data_cnt < 0);
	zassert_true(bss_cnt < 3);
}
EXPORT_SYMBOL(detached_entry);

void test_entry(void)
{
	detached_entry();
}
EXPORT_SYMBOL(test_entry);
