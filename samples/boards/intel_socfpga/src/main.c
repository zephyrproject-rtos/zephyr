/*
 * Copyright (c) 2022-2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

void main(void)
{
	printk("%s: Starting Shell...\n", CONFIG_BOARD);
}
