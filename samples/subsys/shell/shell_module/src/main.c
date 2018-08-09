/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <shell/legacy_shell.h>
#include <version.h>

static int shell_cmd_ping(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("pong\n");

	return 0;
}

static int shell_cmd_params(int argc, char *argv[])
{
	int cnt;

	printk("argc = %d\n", argc);
	for (cnt = 0; cnt < argc; cnt++) {
		printk("  argv[%d] = %s\n", cnt, argv[cnt]);
	}
	return 0;
}

#define SHELL_CMD_VERSION "version"
static int shell_cmd_version(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("Zephyr version %s\n", KERNEL_VERSION_STRING);
	return 0;
}

SHELL_REGISTER_COMMAND(SHELL_CMD_VERSION, shell_cmd_version,
		       "Show kernel version");

#define MY_SHELL_MODULE "sample_module"

static struct shell_cmd commands[] = {
	{ "ping", shell_cmd_ping, NULL },
	{ "params", shell_cmd_params, "print argc" },
	{ NULL, NULL, NULL }
};


void main(void)
{
	SHELL_REGISTER(MY_SHELL_MODULE, commands);
}
