/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <zephyr/drivers/pm_cpu_ops.h>

static int cmd_reboot_warm(const struct shell *shctx, size_t argc, char **argv)
{
	ARG_UNUSED(shctx);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int ret = 0;

	ret = pm_system_reset(SYS_WARM_RESET);

	return ret;
}

static int cmd_reboot_cold(const struct shell *shctx, size_t argc, char **argv)
{
	ARG_UNUSED(shctx);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int ret = 0;

	ret = pm_system_reset(SYS_COLD_RESET);

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_reboot,
		SHELL_CMD_ARG(warm, NULL, "psci warm", cmd_reboot_warm, 1, 0),
		SHELL_CMD_ARG(cold, NULL, "psci cold", cmd_reboot_cold, 1, 0),
		SHELL_SUBCMD_SET_END /* Array terminated. */
		);

SHELL_CMD_REGISTER(psci, &sub_reboot, "PSCI commands", NULL);
