/*
 * Copyright © 2021, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

__weak void _exit(int status)
{
	printf("exit\n");
	while (1) {
		Z_SPIN_DELAY(100);
	}
}
