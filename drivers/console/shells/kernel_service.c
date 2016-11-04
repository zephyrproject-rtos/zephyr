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

#include <misc/printk.h>
#include <misc/shell.h>
#include <init.h>

#define SHELL_KERNEL "kernel"

static int shell_cmd_version(int argc, char *argv[])
{
	uint32_t version = sys_kernel_version_get();

	printk("Zephyr version %d.%d.%d\n",
	       SYS_KERNEL_VER_MAJOR(version),
	       SYS_KERNEL_VER_MINOR(version),
	       SYS_KERNEL_VER_PATCHLEVEL(version));
	return 0;
}

static int shell_cmd_uptime(int argc, char *argv[])
{
	printk("uptime: %u ms\n", k_uptime_get_32());

	return 0;
}

static int shell_cmd_cycles(int argc, char *argv[])
{
	printk("cycles: %u hw cycles\n", k_cycle_get_32());

	return 0;
}


struct shell_cmd kernel_commands[] = {
	{ "version", shell_cmd_version, "show kernel version" },
	{ "uptime", shell_cmd_uptime, "show system uptime in milliseconds" },
	{ "cycles", shell_cmd_cycles, "show system hardware cycles" },
	{ NULL, NULL }
};


SHELL_REGISTER(SHELL_KERNEL, kernel_commands);
