/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <kernel_internal.h>

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/debug/symtab.h>
#include <zephyr/kernel.h>

struct thread_entry {
	const struct k_thread *const thread;
	bool valid;
};

static void thread_valid_cb(const struct k_thread *cthread, void *user_data)
{
	struct thread_entry *entry = user_data;

	if (cthread == entry->thread) {
		entry->valid = true;
	}
}

bool z_thread_is_valid(const struct k_thread *thread)
{
	struct thread_entry entry = {
		.thread = thread,
		.valid = false,
	};

	k_thread_foreach(thread_valid_cb, &entry);

	return entry.valid;
}

SHELL_SUBCMD_SET_CREATE(sub_kernel_thread, (thread));
KERNEL_CMD_ADD(thread, &sub_kernel_thread, "Kernel threads.", NULL);
