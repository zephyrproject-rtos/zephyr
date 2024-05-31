/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>

int main(void)
{
	int Cnt = 0;

	while(true) {
		printf("%d - Hello World! %s with mTimer = %dHz\n", Cnt++, CONFIG_BOARD_TARGET, CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
		k_msleep(1000);
	}

	return 0;
}
