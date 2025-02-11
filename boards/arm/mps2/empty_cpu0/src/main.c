/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

extern void wakeup_cpu1(void);

int main(void)
{
	/* Simply wake-up the remote core */
	wakeup_cpu1();

	while (1) {
	}
	return 0;
}
