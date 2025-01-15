/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <zephyr/logging/log_ctrl.h>

static int cmd_kernel_log_level_set(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int err = 0;

	uint8_t severity = shell_strtoul(argv[2], 10, &err);

	shell_warn(sh, "This command is deprecated as it is a duplicate. "
		       "Use 'log enable' command from logging commands set.");
	if (err) {
		shell_error(sh, "Unable to parse log severity (err %d)", err);

		return err;
	}

	if (severity > LOG_LEVEL_DBG) {
		shell_error(sh, "Invalid log level: %d", severity);
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	int source_id = log_source_id_get(argv[1]);

	/* log_filter_set() takes an int16_t for the source ID */
	if (source_id < 0) {
		shell_error(sh, "Unable to find log source: %s", argv[1]);
		return -EINVAL;
	}

	log_filter_set(NULL, 0, (int16_t)source_id, severity);

	return 0;
}

KERNEL_CMD_ARG_ADD(log_level, NULL, "<module name> <severity (0-4)>", cmd_kernel_log_level_set, 3,
		   0);
