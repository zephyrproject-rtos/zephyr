/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/* Main entry point */
void main(void)
{
	printk("Swapped application booted on %s\n", CONFIG_BOARD);
}
