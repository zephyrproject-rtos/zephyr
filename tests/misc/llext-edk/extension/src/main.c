/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/llext/symbol.h>

#include <app_api.h>

int start(int bar)
{
	printk("foo(%d) is %d\n", bar, foo(bar));
	return 0;
}
EXPORT_SYMBOL(start);
