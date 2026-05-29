/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>

SHELL_SUBCMD_SET_CREATE(posix_cmds, (posix));
SHELL_CMD_ARG_REGISTER(posix, &posix_cmds, "POSIX shell commands", NULL, 2, 0);
