/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_USERSPACE
#include <misc/sem.h>
#include <syscall_handler.h>

void sys_sem_init(struct sys_sem *sem, int initial_count, unsigned int limit)
{
	sem->count = initial_count;
	sem->limit = limit;
}

int sys_sem_give(struct sys_sem *sem)
{
	atomic_val_t oldvalue;

	do {
		oldvalue = atomic_get(&sem->count);

		if (oldvalue >= sem->limit) {
			break;
		}
	} while (atomic_cas(&sem->count, oldvalue, oldvalue + 1) == 0);

	k_futex(&sem->count, FUTEX_WAKE, 1,  K_NO_WAIT);

	return 0;
}

int sys_sem_take(struct sys_sem *sem, s32_t timeout)
{
	atomic_val_t oldvalue;

	do {
		oldvalue = atomic_get(&sem->count);

		if (oldvalue <= 0) {
			break;
		}
	} while (atomic_cas(&sem->count, oldvalue, oldvalue - 1) == 0);

	if (oldvalue > 0) {
		return 0;
	}

	k_futex(&sem->count, FUTEX_WAIT, 0, timeout);

	return 0;
}
#else
#include <misc/sem.h>

void sys_sem_init(struct sys_sem *sem, int initial_count, unsigned int limit)
{
	k_sem_init(&sem->kernel_sem, initial_count, limit);
}

int sys_sem_give(struct sys_sem *sem)
{
	k_sem_give(&sem->kernel_sem);

	return 0;
}

int sys_sem_take(struct sys_sem *sem, s32_t timeout)
{
	k_sem_take(&sem->kernel_sem, timeout);

	return 0;
}
#endif
