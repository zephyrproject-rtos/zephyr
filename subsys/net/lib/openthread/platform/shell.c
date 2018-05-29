/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <stdio.h>
#include <misc/printk.h>
#include <shell/shell.h>
#include <openthread/cli.h>
#include <platform.h>

#include "platform-zephyr.h"

#define OT_SHELL_MODULE "ot"

#define OT_SHELL_BUFFER_SIZE 256

static char rx_buffer[OT_SHELL_BUFFER_SIZE];

int otConsoleOutputCallback(const char *aBuf, uint16_t aBufLength,
			    void *aContext)
{
	ARG_UNUSED(aContext);

	printk("%s", aBuf);

	return aBufLength;
}

static int ot_cmd(int argc, char *argv[])
{
	char *buf_ptr = rx_buffer;
	size_t buf_len = OT_SHELL_BUFFER_SIZE;
	size_t arg_len = 0;
	int i;

	if (argc < 2) {
		return -1;
	}

	for (i = 1; i < argc; i++) {
		if (arg_len) {
			buf_len -= arg_len + 1;
			if (buf_len) {
				buf_ptr[arg_len] = ' ';
			}
			buf_ptr += arg_len + 1;
		}

		arg_len = snprintf(buf_ptr, buf_len, "%s", argv[i]);

		if (arg_len >= buf_len) {
			printk("OT shell buffer full\n");
			return -1;
		}
	}

	if (i == argc) {
		buf_len -= arg_len;
	}

	otCliConsoleInputLine(rx_buffer, OT_SHELL_BUFFER_SIZE - buf_len);

	return 0;
}

static struct shell_cmd ot_commands[] = {
	{ "cmd", ot_cmd, "OpenThread command" },
	{NULL, NULL, NULL}
};

void platformShellInit(otInstance *aInstance)
{
	SHELL_REGISTER(OT_SHELL_MODULE, ot_commands);
	otCliConsoleInit(aInstance, otConsoleOutputCallback, NULL);
}

