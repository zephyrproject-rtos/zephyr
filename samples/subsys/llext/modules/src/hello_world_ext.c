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

#include <zephyr/llext/symbol.h>
#include <zephyr/sys/printk.h>

void hello_world(void)
{
#if defined(CONFIG_HELLO_WORLD_MODE)
	/* HELLO_WORLD_MODE=y: CONFIG_* is defined */
	printk("Hello, world, from the main binary!\n");
#else
	/* HELLO_WORLD_MODE=m: CONFIG_*_MODULE is defined instead */
	printk("Hello, world, from an llext!\n");
#endif
}
EXPORT_SYMBOL(hello_world);
