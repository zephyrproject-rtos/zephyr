/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	printk("hwspinlock shell sample ready\n");
	printk("Type 'hwspinlock' for available commands.\n");

	while (1) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
