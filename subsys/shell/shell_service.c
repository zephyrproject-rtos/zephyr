/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Shell framework services
 *
 * This module initiates shell.
 * Includes also adding basic framework commands to shell commands.
 */

#include <misc/printk.h>
#include <shell/shell.h>
#include <init.h>

#define SHELL_PROMPT "shell> "

int shell_run(struct device *dev)
{
	ARG_UNUSED(dev);

	shell_init(SHELL_PROMPT);
	return 0;
}


SYS_INIT(shell_run, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
