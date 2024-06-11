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
#include <zephyr/ztest_assert.h>

/* Test non-static global object relocation */
int number = 0x42;
extern int ext_number;
int ext_sum_fn(int arg);

int run_id = 41;

void test_entry(void)
{
	switch (run_id) {
	case 41:
		/* initial run: test variable initialization */
		break;
	case 42:
		/* user-mode run: reinit number */
		number = 0x42;
		break;
	default:
		/* possible llext loader issue */
		zassert_unreachable("unexpected run_id %d", run_id);
		return;
	}

	printk("initial: local %d plus external %d equals %d\n",
	       number, ext_number, ext_sum_fn(ext_number));
	zassert_equal(number, 0x42);
	zassert_equal(ext_number, 0x18);
	number ^= ext_number;
	ext_number ^= number;
	number ^= ext_number;
	zassert_equal(number, 0x18);
	zassert_equal(ext_number, 0x42);
	printk("updated: local %d plus external %d equals %d\n",
	       number, ext_number, ext_sum_fn(ext_number));

	run_id += 1;
}
LL_EXTENSION_SYMBOL(test_entry);
