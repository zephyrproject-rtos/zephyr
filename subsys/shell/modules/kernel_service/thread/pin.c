/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <zephyr/kernel.h>

static int cmd_kernel_thread_pin(const struct shell *sh,
				  size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int cpu, err = 0;
	struct k_thread *thread;

	thread = UINT_TO_POINTER(shell_strtoull(argv[1], 16, &err));
	if (err != 0) {
		shell_error(sh, "Unable to parse thread ID %s (err %d)", argv[1], err);
		return err;
	}

	if (!z_thread_is_valid(thread)) {
		shell_error(sh, "Invalid thread id %p", (void *)thread);
		return -EINVAL;
	}

	cpu = shell_strtoul(argv[2], 10, &err);
	if (err != 0) {
		shell_error(sh, "Unable to parse CPU ID %s (err %d)", argv[2], err);
		return err;
	}

	shell_print(sh, "Pinning %p %s to CPU %d", (void *)thread, thread->name, cpu);
	err = k_thread_cpu_pin(thread, cpu);
	if (err != 0) {
		shell_error(sh, "Failed - %d", err);
	} else {
		shell_print(sh, "%p %s cpu_mask: 0x%x", (void *)thread, thread->name,
			    thread->base.cpu_mask);
	}

	return err;
}

KERNEL_THREAD_CMD_ARG_ADD(pin, NULL,
			  "Pin thread to a CPU.\n"
			  "Usage: kernel pin <thread ID> <CPU ID>",
			  cmd_kernel_thread_pin, 3, 0);
