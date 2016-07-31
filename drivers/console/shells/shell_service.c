/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Shell framework services
 *
 * This module initiates shell.
 * Includes also adding basic framework commands to shell commands.
 */

#ifndef _SHELLSERVICE_H
#define _SHELLSERVICE_H

#include <misc/printk.h>
#include <misc/shell.h>
#include <init.h>

#define SHELL_KERNEL "kernel"
#define SHELL_PROMPT "shell> "

static int shell_cmd_version(int argc, char *argv[])
{
	uint32_t version = sys_kernel_version_get();

	printk("Zephyr version %d.%d.%d\n",
		SYS_KERNEL_VER_MAJOR(version),
		SYS_KERNEL_VER_MINOR(version),
		SYS_KERNEL_VER_PATCHLEVEL(version));
	return 0;
}


struct shell_cmd kernel_commands[] = {
	{ "version", shell_cmd_version, "show kernel version" },
	{ NULL, NULL }
};


int shell_run(struct device *dev)
{
	ARG_UNUSED(dev);

	shell_init(SHELL_PROMPT);
	SHELL_REGISTER(SHELL_KERNEL, kernel_commands);
	return 0;
}


SYS_INIT(shell_run, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* SHELLSERVICE_H */
