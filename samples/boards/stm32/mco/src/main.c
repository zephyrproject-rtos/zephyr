/*
 * Copyright (c) 2024 Joakim Andersson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>

int main(void)
{
	printk("\nMCO pin configured, end of example.\n");
	return 0;
}
