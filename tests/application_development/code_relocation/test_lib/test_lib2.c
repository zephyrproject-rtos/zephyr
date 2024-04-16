/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

void relocated_helper(void)
{
	printk("Relocated helper function running on %s\n\n", CONFIG_BOARD);
}
