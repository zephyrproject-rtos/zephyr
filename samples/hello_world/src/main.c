/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>


/* 1000 msec = 1 sec */
#define SLEEP_LONG_TIME_MS   1000


void main(void)
{
	int		Count;


	Count = 0;
	while (1) {
		Count++;
		printk("Hello World! %s pass %8d\n", CONFIG_BOARD, Count);
		k_msleep(SLEEP_LONG_TIME_MS);
	}
}
