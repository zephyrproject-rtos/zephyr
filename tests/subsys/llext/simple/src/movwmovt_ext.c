/*
 * Copyright (c) 2024 Schneider Electric.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This test is designed to test MOV.W and MOV.T instructions on ARM architectures.
 * (except Cortex-M0, M0+ and M1, that don't support them)
 */

#include <stdint.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest_assert.h>

static int test_var;

static __used void test_func(void)
{
	printk("%s\n", __func__);
	test_var = 1;
}

void test_entry(void)
{
	test_var = 0;

	printk("test movwmovt\n");
	__asm volatile ("movw r0, #:lower16:test_func");
	__asm volatile ("movt r0, #:upper16:test_func");
	__asm volatile ("blx r0");
	zassert_equal(test_var, 1, "mov.w and mov.t test failed");
}
LL_EXTENSION_SYMBOL(test_entry);
