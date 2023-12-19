/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This very simple hello world C code can be used as a test case for building
 * probably the simplest loadable extension. It requires a single symbol be
 * linked, section relocation support, and the ability to export and call out to
 * a function.
 */

#include <stdint.h>
#include <zephyr/llext/symbol.h>

extern void printk(char *fmt, ...);

static const uint32_t static_const = 1; /* .text, file-local linkage */
static uint32_t static_var = 2;         /* .data, file-local linkage */
uint32_t static_bss; /* = 3 */          /* .bss,  file-local linkage */

const uint32_t global_const = 4;        /* .text, global linkage */
uint32_t global_var = 5;                /* .data, global linkage */
uint32_t global_bss; /* = 6 */          /* .bss,  global linkage */

extern void hello_world(void)
{
	printk("hello world\n");

	/* Set BSS variables to their expected values */
	static_bss = 3;
	global_bss = 6;

	/* Print all defined variables in sequence */
	printk("Testing number sequence:");
	printk(" %lu", static_const);
	printk(" %lu", static_var);
	printk(" %lu", static_bss);
	printk(" %lu", global_const);
	printk(" %lu", global_var);
	printk(" %lu\n", global_bss);

	/* Print linked variable addresses by group */
	printk(".text variables: %p %p\n", &static_const, &global_const);
	printk(".data variables: %p %p\n", &static_var, &global_var);
	printk(".bss  variables: %p %p\n", &static_bss, &global_bss);
}
LL_EXTENSION_SYMBOL(hello_world);
