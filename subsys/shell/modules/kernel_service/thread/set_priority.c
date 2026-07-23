/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include "kernel_shell.h"

static int cmd_kernel_thread_set_priority(const struct shell *sh, size_t argc, char **argv)
{
	struct k_thread *thread;
	long prio;
	int err;

	thread = UINT_TO_POINTER(shell_strtoull(argv[1], 16, &err));
	if (err) {
		shell_error(sh, "Unable to parse thread id %s", argv[1]);
		return err;
	}

	if (!z_thread_is_valid(thread)) {
		shell_error(sh, "Invalid thread id %p", (void *)thread);
		return -EINVAL;
	}

	prio = shell_strtol(argv[2], 10, &err);
	if (err != 0) {
		shell_error(sh, "Unable to parse priority %s", argv[2]);
		return err;
	}

	if (prio < -(long)CONFIG_NUM_COOP_PRIORITIES ||
	    prio >= (long)CONFIG_NUM_PREEMPT_PRIORITIES) {
		shell_error(sh, "Invalid priority %ld", prio);
		return -EINVAL;
	}

	k_thread_priority_set(thread, prio);
	return 0;
}

KERNEL_THREAD_CMD_ARG_ADD(set_priority, NULL,
			  "Set thread priority.\n"
			  "Usage: kernel thread set_priority <thread ID> <priority>",
			  cmd_kernel_thread_set_priority, 3, 0);
