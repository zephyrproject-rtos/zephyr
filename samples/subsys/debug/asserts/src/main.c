/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/zassert.h>

ZASSERT_MODULE(MYMODULE);

int main(void)
{
	printk("Granular assert sample\n");
	printk("MYMODULE assert level        = %d\n", CONFIG_ASSERT_MODULE_MYMODULE_LEVEL);

	ZASSERT(1 == 1, "passing module assertion is a no-op");
	printk("Passed module ZASSERT(1 == 1)\n");

	printk("Triggering a failing module ZASSERT() ...\n");

	int x = 2;
	ZASSERT(x == 3, "x was %d, expected 3", x);

	return 0;
}
