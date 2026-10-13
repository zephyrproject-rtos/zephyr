/* Copyright (c) Zephy Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sem.h>
#include <wait_q.h>
#include <ksem.h>

int sys_sem_init(struct sys_sem *sem, unsigned int initial_count,
		 unsigned int limit)
{
	k_sem_init(&sem->kernel_sem, initial_count, limit);

	return 0;
}

int sys_sem_give(struct sys_sem *sem)
{
	if (z_sem_give_checked(&sem->kernel_sem) < 0) {
		return -EAGAIN;
	}

	return 0;
}

int sys_sem_take(struct sys_sem *sem, k_timeout_t timeout)
{
	int ret_value = 0;

	ret_value = k_sem_take(&sem->kernel_sem, timeout);
	if ((ret_value == -EAGAIN) || (ret_value == -EBUSY)) {
		ret_value = -ETIMEDOUT;
	}

	return ret_value;
}

unsigned int sys_sem_count_get(struct sys_sem *sem)
{
	return k_sem_count_get(&sem->kernel_sem);
}

bool sys_sem_has_waiters(struct sys_sem *sem)
{
	return z_waitq_head(&sem->kernel_sem.wait_q) != NULL;
}
