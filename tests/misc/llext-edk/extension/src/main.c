/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <app_api.h>

int start(void)
{
	int bar = 42;

	printk("foo(%d) is %d\n", bar, foo(bar));
	return 0;
}
