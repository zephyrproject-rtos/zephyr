/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>

/* filled in by each of the protocol modules */
SHELL_SUBCMD_SET_CREATE(scmi_cmds, (scmi));

SHELL_CMD_REGISTER(scmi, &scmi_cmds, "ARM SCMI commands", NULL);
