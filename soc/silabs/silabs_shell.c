/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/shell/shell.h>

SHELL_SUBCMD_SET_CREATE(silabs_cmds, (silabs));
SHELL_CMD_REGISTER(silabs, &silabs_cmds, "Silicon Labs specific commands", NULL);
