/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <shell/shell.h>

static int shell_cmd_ping(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("pong\n");

	return 0;
}

static int shell_cmd_params(int argc, char *argv[])
{
	printk("argc = %d, argv[0] = %s\n", argc, argv[0]);

	return 0;
}

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
