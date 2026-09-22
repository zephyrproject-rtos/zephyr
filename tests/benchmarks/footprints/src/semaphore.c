/*
 * Copyright (c) 2017-2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>

#include "footprint.h"

#define STACK_SIZE	512

K_SEM_DEFINE(semaphore0, 0, 1);

void thread_fn(void *p1, void *p2, void *p3)
{
	k_sem_give(&semaphore0);

	k_sem_take(&semaphore0, K_FOREVER);
}

void run_semaphore(void)
{
	k_tid_t sem0_tid;
	struct k_sem sem0;

	k_sem_init(&sem0, 0, 1);

	k_sem_give(&sem0);

	k_sem_take(&sem0, K_FOREVER);

	sem0_tid = k_thread_create(&my_thread, my_stack_area,
				   STACK_SIZE, thread_fn,
				   NULL, NULL, NULL,
				   0, 0, K_NO_WAIT);

	k_thread_join(sem0_tid, K_FOREVER);

#ifdef CONFIG_USERSPACE
	sem0_tid = k_thread_create(&my_thread, my_stack_area,
				   STACK_SIZE, thread_fn,
				   NULL, NULL, NULL,
				   0, K_USER, K_FOREVER);

	k_object_access_grant(&semaphore0, sem0_tid);

	k_thread_start(sem0_tid);
	k_thread_join(sem0_tid, K_FOREVER);
#endif
}
