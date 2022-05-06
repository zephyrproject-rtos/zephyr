/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>

extern void wakeup_cpu1(void);

void main(void)
{
	/* Simply wake-up the remote core */
	wakeup_cpu1();

	while (1) {
	}
}
