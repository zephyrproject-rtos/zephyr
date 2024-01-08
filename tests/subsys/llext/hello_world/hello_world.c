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

extern void printk(char *fmt, ...);

static const uint32_t number = 42;

extern void hello_world(void)
{
	printk("hello world\n");
	printk("A number is %lu\n", number);
}
