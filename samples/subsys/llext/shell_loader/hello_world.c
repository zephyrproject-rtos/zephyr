/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This very simple hello world C code can be used while learning about llext.
 */

extern void printk(const char *fmt, ...);

void hello_world(void)
{
	printk("hello world\n");
}
