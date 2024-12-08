/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <zephyr/debug/symtab.h>
#include <zephyr/kernel.h>

static bool print_trace_address(void *arg, unsigned long ra)
{
	const struct shell *sh = arg;
#ifdef CONFIG_SYMTAB
	uint32_t offset = 0;
	const char *name = symtab_find_symbol_name(ra, &offset);

	shell_print(sh, "ra: %p [%s+0x%x]", (void *)ra, name, offset);
#else
	shell_print(sh, "ra: %p", (void *)ra);
#endif

	return true;
}

static int cmd_kernel_thread_unwind(const struct shell *sh, size_t argc, char **argv)
{
	struct k_thread *thread;
	int err = 0;

	if (argc == 1) {
		thread = arch_current_thread();
	} else {
		thread = UINT_TO_POINTER(shell_strtoull(argv[1], 16, &err));
		if (err != 0) {
			shell_error(sh, "Unable to parse thread ID %s (err %d)", argv[1], err);
			return err;
		}

		if (!z_thread_is_valid(thread)) {
			shell_error(sh, "Invalid thread id %p", (void *)thread);
			return -EINVAL;
		}
	}
	shell_print(sh, "Unwinding %p %s", (void *)thread, thread->name);

	arch_stack_walk(print_trace_address, (void *)sh, thread, NULL);

	return 0;
}

KERNEL_THREAD_CMD_ARG_ADD(unwind, NULL, "Unwind a thread.", cmd_kernel_thread_unwind, 1, 1);
