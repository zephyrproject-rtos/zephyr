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

LOG_MODULE_REGISTER(app);

extern void foo(void);

void timer_expired_handler(struct k_timer *timer)
{
	LOG_INF("Timer expired.");

	/* Call another module to present logging from multiple sources. */
	foo();
}

K_TIMER_DEFINE(log_timer, timer_expired_handler, NULL);

static int cmd_log_test_start(const struct shell *shell, size_t argc,
			      char **argv, u32_t period)
{
	ARG_UNUSED(argv);

	int err = shell_cmd_precheck(shell, argc == 1, NULL, 0);

	if (err) {
		return err;
	}

	k_timer_start(&log_timer, period, period);
	shell_fprintf(shell, SHELL_NORMAL, "Log test started\n");
	return 0;
}

static int cmd_log_test_start_demo(const struct shell *shell, size_t argc,
				   char **argv)
{
	cmd_log_test_start(shell, argc, argv, 200);
	return 0;
}

static int cmd_log_test_start_flood(const struct shell *shell, size_t argc,
				    char **argv)
{
	cmd_log_test_start(shell, argc, argv, 10);
	return 0;
}

static int cmd_log_test_stop(const struct shell *shell, size_t argc,
			     char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err = shell_cmd_precheck(shell, argc == 1, NULL, 0);

	if (err) {
		return err;
	}

	k_timer_stop(&log_timer);
	shell_fprintf(shell, SHELL_NORMAL, "Log test stopped\n");

	return 0;
}
SHELL_CREATE_STATIC_SUBCMD_SET(sub_log_test_start)
{
	/* Alphabetically sorted. */
	SHELL_CMD(demo, NULL,
		  "Start log timer which generates log message every 200ms.",
		  cmd_log_test_start_demo),
	SHELL_CMD(flood, NULL,
		  "Start log timer which generates log message every 10ms.",
		  cmd_log_test_start_flood),
	SHELL_SUBCMD_SET_END /* Array terminated. */
};
SHELL_CREATE_STATIC_SUBCMD_SET(sub_log_test)
{
	/* Alphabetically sorted. */
	SHELL_CMD(start, &sub_log_test_start, "Start log test", NULL),
	SHELL_CMD(stop, NULL, "Stop log test.", cmd_log_test_stop),
	SHELL_SUBCMD_SET_END /* Array terminated. */
};

SHELL_CMD_REGISTER(log_test, &sub_log_test, "Log test", NULL);

static int cmd_demo_ping(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_NORMAL, "pong\n");

	return 0;
}

static int cmd_demo_params(const struct shell *shell, size_t argc, char **argv)
{
	int cnt;

	shell_fprintf(shell, SHELL_NORMAL, "argc = %d\n", argc);
	for (cnt = 0; cnt < argc; cnt++) {
		shell_fprintf(shell, SHELL_NORMAL,
				"  argv[%d] = %s\n", cnt, argv[cnt]);
	}
	return 0;
}

static int cmd_version(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_NORMAL,
		      "Zephyr version %s\n", KERNEL_VERSION_STRING);
	return 0;
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

}
