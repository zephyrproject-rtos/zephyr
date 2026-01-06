/*
 * Copyright (C) 2019-2024 Alibaba Group Holding Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/kernel.h>
#include <stdlib.h>

int main(void)
{
	printk("Hello World Sample Begin\n");
	int cnt = 10;

	printk("Hello World! %s\n", CONFIG_BOARD);
	while (cnt) {
		k_msleep(1000);
		printk("Hello World! %s, cnt = %d, cpu_id: %d\n", CONFIG_BOARD, cnt--,
		arch_curr_cpu()->id);
	}

	printk("Hello World Sample End\n");

	return 0;
}
