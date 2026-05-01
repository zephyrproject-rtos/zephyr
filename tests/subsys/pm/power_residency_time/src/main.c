/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#define DIFF_RESIDENCY_TIME_STATE_0 600
#define DIFF_RESIDENCY_TIME_STATE_1 600
#define DIFF_RESIDENCY_TIME_STATE_2 600

#define PM_STATE0_NODE \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(stop0)), (DT_NODELABEL(stop0)), \
		     (DT_NODELABEL(idle)))
#define PM_STATE1_NODE \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(stop1)), (DT_NODELABEL(stop1)), \
		     (DT_NODELABEL(suspend)))
#define PM_STATE2_NODE \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(stop2)), (DT_NODELABEL(stop2)), \
		     (DT_NODELABEL(standby)))

int main(void)
{

	int stop0_min_residency_time = DT_PROP(PM_STATE0_NODE, min_residency_us);
	int stop1_min_residency_time = DT_PROP(PM_STATE1_NODE, min_residency_us);
	int stop2_min_residency_time = DT_PROP(PM_STATE2_NODE, min_residency_us);

	while (1) {
		printk("\nSleep time < min_residency_time of state 0\n");
		k_usleep(stop0_min_residency_time - DIFF_RESIDENCY_TIME_STATE_0);
		printk("\nSleep time = min_residency_time of state 0\n");
		k_usleep(stop0_min_residency_time);
		printk("\nSleep time < min_residency_time of state 1\n");
		k_usleep(stop1_min_residency_time - DIFF_RESIDENCY_TIME_STATE_1);
		printk("\nSleep time = min_residency_time of state 1\n");
		k_usleep(stop1_min_residency_time);
		printk("\nSleep time < min_residency_time of state 2\n");
		k_usleep(stop2_min_residency_time - DIFF_RESIDENCY_TIME_STATE_2);
		printk("\nSleep time = min_residency_time of state 2\n");
		k_usleep(stop2_min_residency_time);
#ifdef CONFIG_ENABLE_PM_MEASURE
		k_msleep(CONFIG_PM_STATE_SUSTAIN_DURATION * 1000);
#endif
		printk("\nWakeup.\n");

		/**
		 * Keeping alive loop
		 */
		while (1) {
			arch_nop();
		}
	}
	return 0;
}
