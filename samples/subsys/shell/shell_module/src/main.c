/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <version.h>
#include <logging/log.h>
#include <stdlib.h>

#define LOG_MODULE_NAME app
LOG_MODULE_REGISTER();

SHELL_UART_DEFINE(shell_transport_uart);
SHELL_DEFINE(uart_shell, "uart:~$ ", &shell_transport_uart, '\r', 10);

void timer_expired_handler(struct k_timer *timer)
{
	LOG_INF("Timer expired.");
}

K_TIMER_DEFINE(log_timer, timer_expired_handler, NULL);

static void cmd_log_test_start(const struct shell *shell, size_t argc, char **argv)
{
	int period;

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	period = strtol(argv[1], NULL, 10);

	k_timer_start(&log_timer, period, period);
	shell_fprintf(shell, SHELL_NORMAL, "Log test started\r\n");
}

static void cmd_log_test_stop(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!shell_cmd_precheck(shell, argc == 1, NULL, 0)) {
		return;
	}
	k_timer_stop(&log_timer);
	shell_fprintf(shell, SHELL_NORMAL, "Log test stoped\r\n");
}

SHELL_CREATE_STATIC_SUBCMD_SET(sub_log_test)
{
	/* Alphabetically sorted. */
	SHELL_CMD(start, NULL,
		  "Start log test providing logging period as a parameter"
		  "(e.g. log_test start 10).",
		  cmd_log_test_start),
	SHELL_CMD(stop, NULL, "Stop log test.", cmd_log_test_stop),
	SHELL_SUBCMD_SET_END /* Array terminated. */
};

SHELL_CMD_REGISTER(log_test, &sub_log_test, "Log test", NULL);

static void cmd_demo_ping(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_NORMAL, "pong\r\n");
}

static void cmd_demo_params(const struct shell *shell, size_t argc, char **argv)
{
	int cnt;

	shell_fprintf(shell, SHELL_NORMAL, "argc = %d\r\n", argc);
	for (cnt = 0; cnt < argc; cnt++) {
		shell_fprintf(shell, SHELL_NORMAL,
				"  argv[%d] = %s\r\n", cnt, argv[cnt]);
	}
}

static void cmd_version(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_NORMAL,
		      "Zephyr version %s\r\n", KERNEL_VERSION_STRING);
}

SHELL_CREATE_STATIC_SUBCMD_SET(sub_demo)
{
	/* Alphabetically sorted. */
	SHELL_CMD(params, NULL, "Print params command.", cmd_demo_params),
	SHELL_CMD(ping, NULL, "Ping command.", cmd_demo_ping),
	SHELL_SUBCMD_SET_END /* Array terminated. */
};

SHELL_CMD_REGISTER(demo, &sub_demo, "Demo commands", NULL);

SHELL_CMD_REGISTER(version, NULL, "Show kernel version", cmd_version);


void main(void)
{
	(void)shell_init(&uart_shell, NULL, true, true, LOG_LEVEL_INF);
}
