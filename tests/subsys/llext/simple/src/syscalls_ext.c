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

#include "syscalls_ext.h"

void test_entry(void)
{
	int input = 41;

	printk("Input: %d Expected output: %d Actual output: %d\n",
	       input, input + 1, ext_syscall_ok(input));
}
LL_EXTENSION_SYMBOL(test_entry);
