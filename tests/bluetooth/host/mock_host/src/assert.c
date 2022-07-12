/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/byteorder.h>

void assert_print(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk(fmt, ap);
	va_end(ap);
}

__attribute__((weak)) void assert_post_action(const char *file, unsigned int line)
{
	printk("Assert error expected as part of test case.\n");
	ztest_test_fail();
}
