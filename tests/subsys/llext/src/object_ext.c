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
#include <zephyr/ztest_assert.h>

/* Test non-static global object relocation */
int number = 42;
const char *string = "hello";

int run_id = 41;

void test_entry(void)
{
	switch (run_id) {
	case 41:
		/* initial run: test variable initialization */
		break;
	case 42:
		/* user-mode run: reinit number */
		number = 42;
		break;
	default:
		/* possible llext loader issue */
		zassert_unreachable("unexpected run_id %d", run_id);
		return;
	}

	printk("number: %d\n", number);
	zassert_equal(number, 42);
	number = 0;
	printk("number, updated: %d\n", number);
	zassert_equal(number, 0);
	printk("string: %s\n", string);
	zassert_ok(strcmp(string, "hello"));

	run_id += 1;
}
EXPORT_SYMBOL(test_entry);
