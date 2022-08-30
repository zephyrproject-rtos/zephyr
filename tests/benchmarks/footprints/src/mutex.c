/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "footprint.h"

#define STACK_SIZE	512

K_MUTEX_DEFINE(user_mutex);

#ifdef CONFIG_USERSPACE
static void user_thread_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_mutex_lock(&user_mutex, K_FOREVER);

	k_mutex_unlock(&user_mutex);
}

static void run_user_mutex(void)
{
	k_tid_t tid;

	/* Exercise simple workqueue */
	tid = k_thread_create(&my_thread, my_stack_area, STACK_SIZE,
			      user_thread_fn, NULL, NULL, NULL,
			      0, K_USER, K_FOREVER);
	k_object_access_grant(&user_mutex, tid);

	k_thread_start(tid);
	k_thread_join(tid, K_FOREVER);
}
#endif /* CONFIG_USERSPACE */

static void run_system_mutex(void)
{
	struct k_mutex sys_mutex;

	k_mutex_init(&sys_mutex);

	k_mutex_lock(&sys_mutex, K_FOREVER);

	k_mutex_unlock(&sys_mutex);
}

void run_mutex(void)
{
	run_system_mutex();

#ifdef CONFIG_USERSPACE
	run_user_mutex();
#endif
}
