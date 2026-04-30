/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <openthread.h>

#include <openthread/cli.h>
#include <openthread/instance.h>

#include "platform-zephyr.h"

#define OT_SHELL_BUFFER_SIZE CONFIG_SHELL_CMD_BUFF_SIZE

static char rx_buffer[OT_SHELL_BUFFER_SIZE];

static const struct shell *shell_p;
static bool is_shell_initialized;

#if defined(CONFIG_OPENTHREAD_SHELL_WAIT_FOR_COMPLETION)
static int32_t last_activity_timestamp;
static K_SEM_DEFINE(completion_sem, 0, 1);
#endif

static int ot_console_cb(void *context, const char *format, va_list arg)
{
	ARG_UNUSED(context);

	if (shell_p == NULL) {
		return 0;
	}

#if defined(CONFIG_OPENTHREAD_SHELL_WAIT_FOR_COMPLETION)
	last_activity_timestamp = k_uptime_get_32();

	/* Inspect output to detect OpenThread command completion */
	char tmp[8];
	va_list arg_copy;

	va_copy(arg_copy, arg);
	vsnprintf(tmp, sizeof(tmp), format, arg_copy);
	va_end(arg_copy);

	/* "Done" is sent as a distinct string */
	bool is_done = (strncmp(tmp, "Done", 4) == 0);
	bool is_error = (strncmp(tmp, "Error ", 6) == 0);

	if (is_done || is_error) {
		k_sem_give(&completion_sem);
	}
#endif
	shell_vfprintf(shell_p, SHELL_NORMAL, format, arg);

	return 0;
}

#define SHELL_HELP_OT	"OpenThread subcommands\n" \
			"Use \"ot help\" to get the list of subcommands"

static int ot_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	char *buf_ptr = rx_buffer;
	size_t buf_len = OT_SHELL_BUFFER_SIZE;
	size_t arg_len = 0;
	int i;

	if (!is_shell_initialized) {
		return -ENOEXEC;
	}

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
			shell_fprintf(sh, SHELL_WARNING,
				      "OT shell buffer full\n");
			return -ENOEXEC;
		}
	}

	if (i == argc) {
		buf_len -= arg_len;
	}

	shell_p = sh;

#if defined(CONFIG_OPENTHREAD_SHELL_WAIT_FOR_COMPLETION)
	last_activity_timestamp = k_uptime_get_32();
	k_sem_reset(&completion_sem);
#endif

	openthread_mutex_lock();
	otCliInputLine(rx_buffer);
	openthread_mutex_unlock();

#if defined(CONFIG_OPENTHREAD_SHELL_WAIT_FOR_COMPLETION)
	int32_t completion_timeout = CONFIG_OPENTHREAD_SHELL_WAIT_FOR_COMPLETION_TIMEOUT;

	/*
	 * Wait for output to finish (signal) or timeout.
	 * If output is received (activity), the timeout is effectively extended.
	 */
	while (true) {
		int32_t time_left = completion_timeout -
				    (k_uptime_get_32() - last_activity_timestamp);

		if (time_left <= 0) {
			break;
		}

		if (k_sem_take(&completion_sem, K_MSEC(time_left)) == 0) {
			break;
		}
	}
#endif

	return 0;
}

SHELL_CMD_ARG_REGISTER(ot, NULL, SHELL_HELP_OT, ot_cmd, 2, 255);

void platformShellInit(otInstance *aInstance)
{
	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL)) {
		shell_p = shell_backend_uart_get_ptr();
	} else {
		shell_p = NULL;
	}

	otCliInit(aInstance, ot_console_cb, NULL);
	is_shell_initialized = true;
}
