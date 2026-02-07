/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/process/builtin.h>
#include <zephyr/kernel.h>

static int entry(size_t argc, const char **argv, struct k_pipe *input, struct k_pipe *output)
{
	printk("enter builtin\n");

	k_sleep(K_SECONDS(1));

	for (size_t i = 0; i < argc; i++) {
		printk("arg%u: %s\n", i, argv[i]);
	}

	printk("exit builtin\n");
	return 0;
}

static struct k_process_builtin builtin;

static int register_builtin(void)
{
	struct k_process *process = k_process_builtin_init(&builtin, "builtin", entry);
	return k_process_register(process);
}

SYS_INIT(register_builtin, POST_KERNEL, 0);
