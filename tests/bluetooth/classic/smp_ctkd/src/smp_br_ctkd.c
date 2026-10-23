/* smp_br_sc_only.c - Bluetooth classic SMP Cross-transport key derivation test */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/shell/shell.h>

#include "common/bt_shell_private.h"
#include "host/settings.h"

static int cmd_reboot(const struct shell *sh, size_t argc, char *argv[])
{
	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}

static int cmd_delete_irk(const struct shell *sh, size_t argc, char *argv[])
{
	bt_settings_delete_irk();

	return 0;
}

static int cmd_default_handler(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(test_smp_cmds,
			       SHELL_CMD_ARG(delete_irk, NULL, HELP_NONE, cmd_delete_irk, 1, 0),
			       SHELL_CMD_ARG(reboot, NULL, HELP_NONE, cmd_reboot, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(test_smp, &test_smp_cmds, "Bluetooth classic SMP shell commands",
		   cmd_default_handler);
