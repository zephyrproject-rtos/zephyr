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


static void test_func(void)
{
	printk("%s\n", __func__);
}

void test_entry(void)
{
	test_func();

	printk("test movwmovt\n");
	__asm volatile ("movw r0, #:lower16:test_func");
	__asm volatile ("movt r0, #:upper16:test_func");
	__asm volatile ("blx r0");
}
LL_EXTENSION_SYMBOL(test_entry);
