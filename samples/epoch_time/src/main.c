/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <sys/time.h>
#include <rtc.h>

#define WAIT_TIME 5
#define MAX_RUN 3
#define RUN_FOREVER 0
#define START_EPOCH_TIME_SEC  1505290000
#define START_EPOCH_TIME_USEC 0

extern int settimeofday(const struct timeval *tv, const struct timezone *tz);

void main(void)
{
	struct timeval v;
	int rv;
	u32_t run = 0;

	/* Set epoch time */
	v.tv_sec = START_EPOCH_TIME_SEC;
	v.tv_usec = START_EPOCH_TIME_USEC;
	rv = settimeofday(&v, NULL);
	if (rv < 0) {
		printk("settimeofday(): ret=%d, errno=%d\n", rv, errno);
		return;
	}

	while (run < MAX_RUN || RUN_FOREVER) {
		k_sleep(WAIT_TIME * MSEC_PER_SEC);
		/* Read epoch time by gettimeoday() */
		rv = gettimeofday(&v, NULL);
		if (rv < 0) {
			printk("gettimeofday(): ret=%d, errno=%d\n", rv, errno);
		}
		printk("Run=%u\n", run);
		printk("gettimeofday(): s=%ld, us=%ld\n", v.tv_sec, v.tv_usec);
		run++;
	}
}
