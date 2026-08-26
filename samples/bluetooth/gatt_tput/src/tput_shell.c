/*
 * Copyright (c) 2026 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Interactive "tput" shell for the central role, mirroring the host client's
 * buttons for the two-board test.
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>

#include "common.h"

#ifdef CONFIG_APP_ROLE_CENTRAL

static int set_target(const struct shell *sh, const char *what, bool on)
{
	if (strcmp(what, "notify") == 0) {
		app_central_set_notify(on);
	} else if (strcmp(what, "write") == 0) {
		app_central_set_write(on);
	} else if (strcmp(what, "both") == 0) {
		app_central_set_notify(on);
		app_central_set_write(on);
	} else {
		shell_error(sh, "expected: notify | write | both");
		return -EINVAL;
	}

	return 0;
}

static int cmd_start(const struct shell *sh, size_t argc, char **argv)
{
	return set_target(sh, argv[1], true);
}

static int cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	return set_target(sh, argv[1], false);
}

static int cmd_throttle(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long kb_s = strtoul(argv[1], NULL, 10);

	if (kb_s > UINT16_MAX) {
		shell_error(sh, "KB/s out of range (0-65535)");
		return -EINVAL;
	}

	app_central_set_throttle((uint16_t)kb_s);
	return 0;
}

static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "connected+discovered: %s", app_central_ready() ? "yes" : "no");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	tput_cmds, SHELL_CMD_ARG(start, NULL, "start notify|write|both", cmd_start, 2, 0),
	SHELL_CMD_ARG(stop, NULL, "stop notify|write|both", cmd_stop, 2, 0),
	SHELL_CMD_ARG(throttle, NULL, "throttle <KB/s> (0 = max)", cmd_throttle, 2, 0),
	SHELL_CMD_ARG(status, NULL, "show link/discovery status", cmd_status, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(tput, &tput_cmds, "BLE throughput control", NULL);

#endif /* CONFIG_APP_ROLE_CENTRAL */
