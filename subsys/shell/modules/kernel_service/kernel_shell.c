/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

SHELL_SUBCMD_SET_CREATE(kernel_cmds, (kernel));
SHELL_CMD_REGISTER(kernel, &kernel_cmds, "Kernel commands", NULL);
