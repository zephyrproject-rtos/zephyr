/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/sem.h>
#include <zephyr/syscall_handler.h>

#ifdef CONFIG_USERSPACE
#define SYS_SEM_MINIMUM      0
#define SYS_SEM_CONTENDED    (SYS_SEM_MINIMUM - 1)

static inline atomic_t bounded_dec(atomic_t *val, atomic_t minimum)
{
	atomic_t old_value, new_value;

	do {
		old_value = atomic_get(val);
		if (old_value < minimum) {
			break;
		}

		new_value = old_value - 1;
	} while (atomic_cas(val, old_value, new_value) == 0);

	return old_value;
}

static inline atomic_t bounded_inc(atomic_t *val, atomic_t minimum,
				   atomic_t maximum)
{
	atomic_t old_value, new_value;

	do {
		old_value = atomic_get(val);
		if (old_value >= maximum) {
			break;
		}

		new_value = old_value < minimum ?
			    minimum + 1 : old_value + 1;
	} while (atomic_cas(val, old_value, new_value) == 0U);

	return old_value;
}

int sys_sem_init(struct sys_sem *sem, unsigned int initial_count,
		 unsigned int limit)
{
	if (sem == NULL || limit == SYS_SEM_MINIMUM ||
	    initial_count > limit || limit > INT_MAX) {
		return -EINVAL;
	}

	atomic_set(&sem->futex.val, initial_count);
	sem->limit = limit;

	return 0;
}

int sys_sem_give(struct sys_sem *sem)
{
	int ret = 0;
	atomic_t old_value;

	old_value = bounded_inc(&sem->futex.val,
				SYS_SEM_MINIMUM, sem->limit);
	if (old_value < 0) {
		ret = k_futex_wake(&sem->futex, true);

		if (ret > 0) {
			return 0;
		}
	} else if (old_value >= sem->limit) {
		return -EAGAIN;
	} else {
		;
	}
	return ret;
}

int sys_sem_take(struct sys_sem *sem, k_timeout_t timeout)
{
	int ret = 0;
	atomic_t old_value;

	do {
		old_value = bounded_dec(&sem->futex.val,
					SYS_SEM_MINIMUM);
		if (old_value > 0) {
			return 0;
		}

		ret = k_futex_wait(&sem->futex,
				   SYS_SEM_CONTENDED, timeout);
	} while (ret == 0 || ret == -EAGAIN);

	return ret;
}

unsigned int sys_sem_count_get(struct sys_sem *sem)
{
	int value = atomic_get(&sem->futex.val);

	return value > SYS_SEM_MINIMUM ? value : SYS_SEM_MINIMUM;
}
#else
int sys_sem_init(struct sys_sem *sem, unsigned int initial_count,
		 unsigned int limit)
{
	k_sem_init(&sem->kernel_sem, initial_count, limit);

	return 0;
}

int sys_sem_give(struct sys_sem *sem)
{
	k_sem_give(&sem->kernel_sem);

	return 0;
}

int sys_sem_take(struct sys_sem *sem, k_timeout_t timeout)
{
	int ret_value = 0;

	ret_value = k_sem_take(&sem->kernel_sem, timeout);
	if (ret_value == -EAGAIN || ret_value == -EBUSY) {
		ret_value = -ETIMEDOUT;
	}

	return ret_value;
}

unsigned int sys_sem_count_get(struct sys_sem *sem)
{
	return k_sem_count_get(&sem->kernel_sem);
}
#endif
