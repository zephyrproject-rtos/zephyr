/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This code demonstrates object relocation support.
 */

#include <stdint.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/kernel.h>

/* Test non-static global object relocation */
int number = 42;
const char *string = "hello";

void test_entry(void)
{
	printk("number: %d\n", number);
	number = 0;
	printk("number, updated: %d\n", number);
	printk("string: %s\n", string);
}
LL_EXTENSION_SYMBOL(test_entry);
