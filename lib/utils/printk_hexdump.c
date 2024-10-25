/*
 * Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>

#include <stdint.h>
#include <stddef.h>

void printk_hexdump(const char *str, const uint8_t *data, size_t data_length)
{
	size_t n = 0;

	if (!data_length) {
		printk("%s has no data\n", str);
		return;
	}

	while (data_length--) {
		if (n % 16 == 0) {
			printk("%s %08zX ", str, n);
		}

		printk("%02X ", *data++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printk("\n");
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		printk("\n");
	}
}
