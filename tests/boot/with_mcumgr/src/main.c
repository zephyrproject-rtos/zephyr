/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/* Main entry point */
int main(void)
{
	printk("Launching primary slot application on %s\n", CONFIG_BOARD);
	return 0;
}
