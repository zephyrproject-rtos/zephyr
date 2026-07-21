/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/console/console.h>

static const char prompt[] = "Start typing characters to see them echoed back\r\n";

int main(void)
{
	console_init();

	printk("You should see another line with instructions below. If not,\n");
	printk("the (interrupt-driven) console device doesn't work as expected:\n");
	console_write(NULL, prompt, sizeof(prompt) - 1);

	while (1) {
		uint8_t c = console_getchar();

		console_putchar(c);
		if (c == '\r') {
			console_putchar('\n');
		}
	}
	return 0;
}
