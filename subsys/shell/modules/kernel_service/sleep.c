/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <zephyr/kernel.h>

static int cmd_kernel_sleep(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	uint32_t ms;
	int err = 0;

	ms = shell_strtoul(argv[1], 10, &err);

	if (!err) {
		k_msleep(ms);
	} else {
		shell_error(sh, "Unable to parse input (err %d)", err);
		return err;
	}

	return 0;
}

KERNEL_CMD_ARG_ADD(sleep, NULL, "ms", cmd_kernel_sleep, 2, 0);
