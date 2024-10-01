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

int main(void)
{
	int ret;

	printk("%s():enter\n", __func__);
	ret = test();
	printk("ret=%d\n", ret);
	return 0;
}
