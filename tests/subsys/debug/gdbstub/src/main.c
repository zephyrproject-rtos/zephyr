/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACKSIZE 512

static int test(void)
{
	int a;
	int b;

	a = 10;
	b = a * 2;

	return a + b;
}

static void thread_entry(void *p1, void *p2, void *p3)
{
	printk("Hello from user thread!\n");
}

int main(void)
{
	int ret;

	ret = test();
	printk("%d\n", ret);
	return 0;
}

K_THREAD_DEFINE(thread, STACKSIZE, thread_entry, NULL, NULL, NULL,
		7, K_USER, 0);
