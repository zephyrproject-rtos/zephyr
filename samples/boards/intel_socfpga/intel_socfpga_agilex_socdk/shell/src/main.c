/*
 * Copyright (c) 2021, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

void main(void)
{
	printk("%s: Starting Shell...\n", CONFIG_BOARD);
}
