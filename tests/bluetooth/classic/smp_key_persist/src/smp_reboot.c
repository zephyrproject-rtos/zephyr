/* smp_br_io_cap.c - Bluetooth classic SMP IO CAP smoke test */

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
#include <zephyr/kernel.h>

#include <zephyr/sys/reboot.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

static int cmd_reboot(const struct shell *sh, size_t argc, char *argv[])
{
	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	test_smp_cmds,
	SHELL_CMD_ARG(reboot, NULL, HELP_NONE, cmd_reboot, 1, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_default_handler(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(test_smp, &test_smp_cmds, "smp test cmds", cmd_default_handler);
