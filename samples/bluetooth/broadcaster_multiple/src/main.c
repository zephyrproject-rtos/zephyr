/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>

int broadcaster_multiple(void);

void main(void)
{
	printk("Starting Multiple Broadcaster Demo\n");

	(void)broadcaster_multiple();

	printk("Exiting %s thread.\n", __func__);
}
