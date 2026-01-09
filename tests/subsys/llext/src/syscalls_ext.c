/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This code demonstrates syscall support.
 */

#include <zephyr/llext/symbol.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest_assert.h>

#include "syscalls_ext.h"

void test_entry(void)
{
	int input = 41;
	int output = ext_syscall_ok(input);

	printk("Input: %d Expected output: %d Actual output: %d\n",
	       input, input + 1, output);
	zassert_equal(output, input + 1);
}
EXPORT_SYMBOL(test_entry);
