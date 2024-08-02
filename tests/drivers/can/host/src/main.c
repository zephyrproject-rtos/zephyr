/*
 * Copyright (c) 2024 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/shell/shell.h>

static const struct device *chosen = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

static int cmd_can_host_chosen(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!device_is_ready(chosen)) {
		shell_error(sh, "zephyr,canbus device %s not ready", chosen->name);
		return -ENODEV;
	}

	shell_print(sh, "zephyr,canbus: %s", chosen->name);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_can_host_cmds,
	SHELL_CMD(chosen, NULL,
		"Get zephyr,canbus chosen device name\n"
		"Usage: can_host chosen",
		cmd_can_host_chosen),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(can_host, &sub_can_host_cmds, "CAN host test commands", NULL);
