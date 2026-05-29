/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <zephyr/kernel.h>

static int cmd_kernel_cycles(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "cycles: %u hw cycles", k_cycle_get_32());
	return 0;
}

KERNEL_CMD_ADD(cycles, NULL, "Kernel cycles.", cmd_kernel_cycles);
