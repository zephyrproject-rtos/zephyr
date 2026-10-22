/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>

int main(void)
{
	printk("Hello world from %s\n", CONFIG_BOARD_TARGET);

	return 0;
}
