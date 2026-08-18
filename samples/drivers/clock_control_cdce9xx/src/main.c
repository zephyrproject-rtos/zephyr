/*
 * Copyright (c) 2026 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h> /* strtol */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/shell/shell.h>

#define CLOCK_DEVICE_NODE DT_ALIAS(clock_control_dev)
const struct device *const clock_dev = DEVICE_DT_GET(CLOCK_DEVICE_NODE);

int main(void)
{
	if (!device_is_ready(clock_dev)) {
		printk("Clock device not ready\n");
		return -ENODEV;
	}
	return 0;
}

static int cmd_clock_on(const struct shell *p_shell_ctx, size_t argc, char **p_argv)
{
	if (argc == 2) {
		int which = strtol(p_argv[1], NULL, 10);
		int rc = clock_control_on(clock_dev, (clock_control_subsys_t)which);

		shell_print(p_shell_ctx, "on for output %d = %d\n", which, rc);
	}

	return 0;
}

static int cmd_clock_off(const struct shell *p_shell_ctx, size_t argc, char **p_argv)
{
	if (argc == 2) {
		int which = strtol(p_argv[1], NULL, 10);
		int rc = clock_control_off(clock_dev, (clock_control_subsys_t)which);

		shell_print(p_shell_ctx, "off for output %d = %d\n", which, rc);
	}

	return 0;
}

static int cmd_get_rate(const struct shell *p_shell_ctx, size_t argc, char **p_argv)
{
	if (argc == 2) {
		int which = strtol(p_argv[1], NULL, 10);
		int rate;
		int rc = clock_control_get_rate(clock_dev, (clock_control_subsys_t)which, &rate);

		if (rc == 0) {
			shell_print(p_shell_ctx, "rate on output %d = %d\n", which, rate);
		} else {
			shell_print(p_shell_ctx, "clock_control_get_rate returnd %d\n", rc);
		}
	}

	return 0;
}

static int cmd_get_status(const struct shell *p_shell_ctx, size_t argc, char **p_argv)
{
	if (argc == 2) {
		int which = strtol(p_argv[1], NULL, 10);
		enum clock_control_status status =
			clock_control_get_status(clock_dev, (clock_control_subsys_t)which);

		shell_print(p_shell_ctx, "status on output %d = %d\n", which, (int)status);
	}

	return 0;
}

static int cmd_set_rate(const struct shell *p_shell_ctx, size_t argc, char **p_argv)
{
	if (argc == 3) {
		int which = strtol(p_argv[1], NULL, 10);
		int rate = strtol(p_argv[2], NULL, 10);
		int rc = clock_control_set_rate(clock_dev, (clock_control_subsys_t)which,
						(clock_control_subsys_rate_t)rate);

		shell_print(p_shell_ctx, "clock_control_set_rate returned %d\n", rc);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_clock_control_cmds,
			       SHELL_CMD_ARG(on, NULL,
					     "Clock On\n"
					     "Usage: on <output>",
					     cmd_clock_on, 2, 0),
			       SHELL_CMD_ARG(off, NULL,
					     "Clock Off\n"
					     "Usage: off <output>",
					     cmd_clock_off, 2, 0),
			       SHELL_CMD_ARG(get_rate, NULL,
					     "Get rate\n"
					     "Usage: get_rate <output>",
					     cmd_get_rate, 2, 0),
			       SHELL_CMD_ARG(get_status, NULL,
					     "Get status\n"
					     "Usage: get_status <output>",
					     cmd_get_status, 2, 0),
			       SHELL_CMD_ARG(set_rate, NULL,
					     "Set rate\n"
					     "Usage: set_rate <output> <rate>",
					     cmd_set_rate, 3, 0),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(clock_control, &sub_clock_control_cmds, "Clock control command", NULL);
