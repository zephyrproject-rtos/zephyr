/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <stdio.h>
#include <net/openthread.h>
#include <sys/printk.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>

#include <openthread/cli.h>
#include <openthread/instance.h>

#include "platform-zephyr.h"

#define OT_SHELL_BUFFER_SIZE CONFIG_SHELL_CMD_BUFF_SIZE

static char rx_buffer[OT_SHELL_BUFFER_SIZE];

static const struct shell *shell_p;

int otConsoleOutputCallback(void *aContext, const char *aFormat,
			    va_list aArguments)
{
	ARG_UNUSED(aContext);

	shell_vfprintf(shell_p, SHELL_NORMAL, aFormat, aArguments);

	return 0;
}

#define SHELL_HELP_OT	"OpenThread subcommands\n" \
			"Use \"ot help\" to get the list of subcommands"

static int ot_cmd(const struct shell *shell, size_t argc, char *argv[])
{
	char *buf_ptr = rx_buffer;
	size_t buf_len = OT_SHELL_BUFFER_SIZE;
	size_t arg_len = 0;
	int i;

	for (i = 1; i < argc; i++) {
		if (arg_len) {
			buf_len -= arg_len + 1;
			if (buf_len) {
				buf_ptr[arg_len] = ' ';
			}
			buf_ptr += arg_len + 1;
		}

		arg_len = snprintk(buf_ptr, buf_len, "%s", argv[i]);

		if (arg_len >= buf_len) {
			shell_fprintf(shell, SHELL_WARNING,
				      "OT shell buffer full\n");
			return -ENOEXEC;
		}
	}

	if (i == argc) {
		buf_len -= arg_len;
	}

	shell_p = shell;

	openthread_api_mutex_lock(openthread_get_default_context());
	otCliInputLine(rx_buffer);
	openthread_api_mutex_unlock(openthread_get_default_context());

	return 0;
}

SHELL_CMD_ARG_REGISTER(ot, NULL, SHELL_HELP_OT, ot_cmd, 2, 255);

void platformShellInit(otInstance *aInstance)
{
	otCliInit(aInstance, otConsoleOutputCallback, NULL);
}
