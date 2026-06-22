/*
 * Copyright (c) 2025 Bang & Olufsen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <zephyr/sys/reboot.h>

static int cmd_kernel_panic(const struct shell *sh,
			    size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	k_panic();

	return 0;
}

KERNEL_CMD_ADD(panic, NULL, "Please, panic now!", cmd_kernel_panic);
