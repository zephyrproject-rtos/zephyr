/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/process/process.h>

static void entry(void *p1, void *p2, void *p3)
{
	struct k_process *process;
	const char *args[] = {"test0", "test1", "test2"};

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("starting subprocesses from %s thread\n",
	       k_is_user_context() ? "user" : "supervisor");

	process = k_process_get("ext");
	k_process_start(process, ARRAY_SIZE(args), args, NULL, NULL);
	process = k_process_get("builtin");
	k_process_start(process, ARRAY_SIZE(args), args, NULL, NULL);
	k_msleep(2000);
}

int main(void)
{
	struct k_process *process;

	/* Run subprocesses as supervisor thread */
	entry(NULL, NULL, NULL);

	/* Grant access to subprocesses to this thread before entering user mode */
	process = k_process_get("ext");
	k_object_access_grant(process, k_current_get());
	process = k_process_get("builtin");
	k_object_access_grant(process, k_current_get());

	/* Run subprocesses as user thread */
	k_thread_user_mode_enter(entry, NULL, NULL, NULL);

	/* We never get here */
	return 0;
}
