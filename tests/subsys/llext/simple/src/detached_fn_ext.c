/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest_assert.h>

__section(".detach") void detached_entry(void)
{
	static unsigned int cnt;

	zassert_true(cnt < 2);
	printk("call #%u\n", cnt++);
}
EXPORT_SYMBOL(detached_entry);

void test_entry(void)
{
	detached_entry();
}
EXPORT_SYMBOL(test_entry);
