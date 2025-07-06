/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

int main(void)
{
	int k_idle_min_residency_time =
		DT_PROP(DT_PATH(zephyr_user), k_idle_state_min_residency_time);
	int stop0_min_residency_time = DT_PROP(DT_NODELABEL(stop0), min_residency_us);
	int stop1_min_residency_time = DT_PROP(DT_NODELABEL(stop1), min_residency_us);
	int stop2_min_residency_time = DT_PROP(DT_NODELABEL(stop2), min_residency_us);

	while (1) {
		printk("\nGoing to k_cpu_idle.\n");
		k_usleep(k_idle_min_residency_time);
		printk("\nWake Up.\n");
		printk("\nGoing to state 0.\n");
		k_usleep(stop0_min_residency_time);
		printk("\nWake Up.\n");
		printk("\nGoing to state 1.\n");
		k_usleep(stop1_min_residency_time);
		printk("\nWake Up.\n");
		printk("\nGoing to state 2.\n");
		k_usleep(stop2_min_residency_time);
		printk("\nWake Up.\n");

		/* Anti-sleeping loop */
		while (1) {
			arch_nop();
		}
	}
	return 0;
}
