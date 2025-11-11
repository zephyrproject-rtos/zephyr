/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <zephyr/version.h>

static int cmd_kernel_version(const struct shell *sh,
			      size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Zephyr version %s", KERNEL_VERSION_STRING);
	return 0;
}

KERNEL_CMD_ADD(version, NULL, "Kernel version.", cmd_kernel_version);
