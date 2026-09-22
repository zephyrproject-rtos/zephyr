/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/console/console.h>

int main(void)
{
	console_getline_init();

	while (1) {
		char *s = console_getline();

		printk("getline: %s;\n", s);
	}
	return 0;
}
