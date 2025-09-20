/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	printk("=== Hello World from Secondary Image ===\n");

	while (1) {
		k_msleep(3000);
		printk("Secondary image heartbeat\n");
	}

	return 0;
}
