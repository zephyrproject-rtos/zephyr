/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/console/console.h>

int main(void)
{
	console_getline_init();

	printk("Enter a line\n");

	while (1) {
		char *s = console_getline();

		printk("line: %s\n", s);
		printk("last char was: 0x%x\n", s[strlen(s) - 1]);
	}
	return 0;
}
