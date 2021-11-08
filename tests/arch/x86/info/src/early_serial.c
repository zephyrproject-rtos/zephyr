/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

extern int arch_printk_char_out(int c);
extern void __printk_hook_install(int (*fn)(int));

/**
 * @brief Test arch_printk_char_out() on x86 platform
 *
 * @details Register function arch_printk_char_out() using
 * __printk_hook_install, then the prink() will use
 * arch_printk_char_out() to put character to console.
 * Check if the character string "Successful!" is send out.
 */
void early_serial(void)
{
	printk("\ntest early serial\n");
	__printk_hook_install(arch_printk_char_out);
}
