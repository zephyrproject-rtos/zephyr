/* Copyright (c) Zephy Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sem.h>
#include <wait_q.h>

int sys_sem_init(struct sys_sem *sem, unsigned int initial_count,
		 unsigned int limit)
{
	k_sem_init(&sem->kernel_sem, initial_count, limit);

	return 0;
}

bool sys_sem_has_waiters(struct sys_sem *sem)
{
	return z_waitq_head(&sem->kernel_sem.wait_q) != NULL;
}
