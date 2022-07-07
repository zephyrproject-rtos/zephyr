/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/logging/log.h>

#include "app_shared.h"
#include "app_a.h"
#include "app_b.h"

#define APP_A_STACKSIZE	2048
#define APP_B_STACKSIZE	2048

LOG_MODULE_REGISTER(app_main);

/* Define a thread for the root of application A.
 */
struct k_thread app_a_thread;
K_THREAD_STACK_DEFINE(app_a_stack, APP_A_STACKSIZE);

/* Define a thread for the root of application B.
 */
struct k_thread app_b_thread;
K_THREAD_STACK_DEFINE(app_b_stack, APP_B_STACKSIZE);

void main(void)
{
	k_tid_t thread_a, thread_b;

	LOG_INF("APP A partition: %p %zu", (void *)app_a_partition.start,
		(size_t)app_a_partition.size);
	LOG_INF("APP B partition: %p %zu", (void *)app_b_partition.start,
		(size_t)app_b_partition.size);
	LOG_INF("Shared partition: %p %zu", (void *)shared_partition.start,
		(size_t)shared_partition.size);
#ifdef Z_LIBC_PARTITION_EXISTS
	LOG_INF("libc partition: %p %zu", (void *)z_libc_partition.start,
		(size_t)z_libc_partition.size);
#endif
	sys_heap_init(&shared_pool, shared_pool_mem, HEAP_BYTES);

	/* Spawn supervisor entry for application A */
	thread_a = k_thread_create(&app_a_thread, app_a_stack, APP_A_STACKSIZE,
				   app_a_entry, NULL, NULL, NULL,
				   -1, K_INHERIT_PERMS, K_NO_WAIT);

	/* Spawn supervisor entry for application B */
	thread_b = k_thread_create(&app_b_thread, app_b_stack, APP_B_STACKSIZE,
				   app_b_entry, NULL, NULL, NULL,
				   -1, K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(thread_a, K_FOREVER);
	k_thread_join(thread_b, K_FOREVER);
}
