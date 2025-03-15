/*
 * Copyright (c) 2024 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This test is designed to test linking global symbols, which for some architectures
 * like ARM generate relative jumps rather than jumping to absolute addresses. Multiple
 * global functions are created to hopefully generate both positive and negative relative
 * jumps.
 */

#include <stdint.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest_assert.h>

static int test_var;

void test_relative_jump_1(void);
void test_relative_jump_2(void);
void test_relative_jump_3(void);
void test_relative_jump_4(void);
void test_relative_jump_5(void);

void test_relative_jump_5(void)
{
	printk("relative jump 5\n");
	test_var = 1;
}

void test_relative_jump_4(void)
{
	printk("relative jump 4\n");
	test_relative_jump_5();
}

void test_relative_jump_2(void)
{
	printk("relative jump 2\n");
	test_relative_jump_3();
}

void test_relative_jump_1(void)
{
	printk("relative jump 1\n");
	test_relative_jump_2();
}

void test_relative_jump_3(void)
{
	printk("relative jump 3\n");
	test_relative_jump_4();
}

void test_entry(void)
{
	printk("enter\n");
	test_var = 0;
	test_relative_jump_1();
	printk("exit\n");
	zassert_equal(test_var, 1, "relative jump test failed");
}
EXPORT_SYMBOL(test_entry);
