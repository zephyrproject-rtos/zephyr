/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <kernel_internal.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdlib.h>

static int cmd_kernel_thread_suspend(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "Usage: thread suspend <0xthread_id>");
		return -EINVAL;
	}
	/* thread_id is converetd from hex to decimal */
	k_tid_t thread_id = (k_tid_t)strtoul(argv[1], NULL, 16);

	if (!z_thread_is_valid(thread_id)) {
		shell_error(shell, "Thread ID %p is not valid", thread_id);
	return -EINVAL;
	}

	k_thread_suspend(thread_id);

	shell_print(shell, "Thread %p suspended", thread_id);

	return 0;
}

KERNEL_THREAD_CMD_ARG_ADD(suspend, NULL, "Suspend threads", cmd_kernel_thread_suspend, 2, 0);
