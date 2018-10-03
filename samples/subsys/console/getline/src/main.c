/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <console.h>

void main(void)
{
	console_getline_init();

	printk("Enter a line\n");

	while (1) {
		char *s = console_getline();

		printk("line: %s\n", s);
		printk("last char was: 0x%x\n", s[strlen(s) - 1]);
	}
}
