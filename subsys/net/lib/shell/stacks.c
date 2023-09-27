/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "common.h"

static int cmd_net_stacks(const struct shell *sh, size_t argc, char *argv[])
{
#if !defined(CONFIG_KERNEL_SHELL)
	PR("Enable CONFIG_KERNEL_SHELL and type \"kernel stacks\" to see stack information.\n");
#else
	PR("Type \"kernel stacks\" to see stack information.\n");
#endif
	return 0;
}

SHELL_SUBCMD_ADD((net), stacks, NULL,
		 "Show network stacks information.",
		 cmd_net_stacks, 1, 0);
