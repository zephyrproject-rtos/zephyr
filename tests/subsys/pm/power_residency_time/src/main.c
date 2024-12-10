/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#define DIFF_RESIDENCY_TIME_STATE_0 250
#define DIFF_RESIDENCY_TIME_STATE_1 250
#define DIFF_RESIDENCY_TIME_STATE_2 250

int main(void)
{

	while (1) {
		printk("\nSleep time below minimum for state 0.\n");
		k_usleep(500000 - DIFF_RESIDENCY_TIME_STATE_0);
		printk("\nSleep time meets minimum for state 0.\n");
		k_usleep(500000);
		printk("\nSleep time below minimum for state 1.\n");
		k_usleep(1000000 - DIFF_RESIDENCY_TIME_STATE_1);
		printk("\nSleep time meets minimum for state 1.\n");
		k_usleep(1000000);
		printk("\nSleep time below minimum for state 2.\n");
		k_usleep(1500000 - DIFF_RESIDENCY_TIME_STATE_2);
		printk("\nSleep time meets minimum for state 2.\n");
		k_usleep(1500000);
		printk("\nWakeup.\n");

		/**
		 * Anti-sleeping loop
		 */
		while (1) {
			arch_nop();
		}
	}
	return 0;
}
