/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This code demonstrates syscall support.
 */

 /* This include directive must appear first in the file.
  *
  * On x86 platforms with demand paging on, Zephyr generates a
  * .pinned_text section containing syscalls. The LLEXT loader
  * requires .text-like sections to appear close to .text at the
  * start of the object file, before .rodata, so they can be
  * grouped into a contiguous text region.
  *
  * Including syscalls_ext.h first ensures the first instance of
  * data to be placed in .pinned_text appears before the first instance
  * of data for .rodata. As a result, .pinned_text will appear
  * before .rodata.
  */
#include "syscalls_ext.h"

#include <zephyr/llext/symbol.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest_assert.h>

void test_entry(void)
{
	int input = 41;
	int output = ext_syscall_ok(input);

	printk("Input: %d Expected output: %d Actual output: %d\n",
	       input, input + 1, output);
	zassert_equal(output, input + 1);
}
EXPORT_SYMBOL(test_entry);
