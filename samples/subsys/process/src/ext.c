/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/llext.h>
#include <zephyr/kernel.h>

int entry(size_t argc, const char **argv, struct k_pipe *input, struct k_pipe *output)
{
	printk("enter ext\n");

	k_sleep(K_MSEC(500));

	for (size_t i = 0; i < argc; i++) {
		printk("arg%u: %s\n", i, argv[i]);
	}

	printk("exit ext\n");
	return 0;
}

EXPORT_SYMBOL(entry);
