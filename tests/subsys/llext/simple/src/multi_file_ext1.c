/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This code demonstrates multi-file object and function linking support.
 */

#include <stdint.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/kernel.h>

/* Test non-static global object relocation */
int number = 0x42;
extern int ext_number;
int ext_sum_fn(int arg);

void test_entry(void)
{
	printk("initial: local %d plus external %d equals %d\n",
	       number, ext_number, ext_sum_fn(ext_number));
	number ^= ext_number;
	ext_number ^= number;
	number ^= ext_number;
	printk("updated: local %d plus external %d equals %d\n",
	       number, ext_number, ext_sum_fn(ext_number));
}
LL_EXTENSION_SYMBOL(test_entry);
