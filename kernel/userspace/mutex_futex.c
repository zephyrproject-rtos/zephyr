/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/mutex.h>
#include <zephyr/internal/syscall_handler.h>

/* Use the LSB of the futex value as indicator that there are waiters;
 * TID is the address of the thread object, which should be 4-byte aligned, and
 * therefore leave the lower two bits free
 */
#define HAS_WAITER BIT(0)
#define TID_MASK   (~HAS_WAITER)

void sys_mutex_init(struct sys_mutex *mutex)
{
	(void)atomic_set(&mutex->futex.val, 0);
	mutex->counter = 0;
}

int sys_mutex_lock(struct sys_mutex *mutex, k_timeout_t timeout)
{
	int ret = 0;
	atomic_t current_value, new_value, target_value;
	k_tid_t tid = k_current_get();
	atomic_t *target = &mutex->futex.val;

	target_value = (atomic_t)(uintptr_t)tid;
	do {
		/* Try to lock mutex; if this works, we are the owner */
		if (atomic_cas(target, 0, target_value)) {
			mutex->counter = 1;
			return 0;
		}

		current_value = atomic_get(target);
		/* In the unlikely event that it got unlocked just after our attempt,
		 * try locking again
		 */
		if (unlikely(current_value == 0)) {
			continue;
		}
		/* Increase counter if owner tries to lock again */
		if ((current_value & TID_MASK) == (atomic_t)(uintptr_t)tid) {
			if (mutex->counter == UINT32_MAX) {
				/* Should never happen */
				return -EFAULT;
			}
			mutex->counter++;
			return 0;
		}

		/* Indicate that the mutex is contended */
		target_value |= HAS_WAITER;
		new_value = current_value | HAS_WAITER;
		/* If no other thread is waiting already, set indicator before starting
		 * to wait, so that the owner will call k_futex_wake when unlocking;
		 * we do not keep track of the number of waiters, which is why the last
		 * owner will unnecessarily call wakeup;
		 * the bit gets cleared when the mutex is acquired again without
		 * contention
		 */
		if (new_value != current_value) {
			(void)atomic_cas(target, current_value, new_value);
		}

		ret = k_futex_wait(&mutex->futex, new_value, timeout);
	} while (ret == 0 || ret == -EAGAIN);

	if (ret == -ETIMEDOUT) {
		ret = K_TIMEOUT_EQ(timeout, K_NO_WAIT) ? -EBUSY : -EAGAIN;
	}

	return ret;
}

int sys_mutex_unlock(struct sys_mutex *mutex)
{
	int ret = 0;
	atomic_t current_value;
	k_tid_t tid = k_current_get();
	atomic_t *target = &mutex->futex.val;

	current_value = atomic_get(target);
	if (unlikely(current_value == 0)) {
		return -EINVAL;
	}
	if (unlikely((current_value & TID_MASK) != (atomic_t)(uintptr_t)tid)) {
		return -EPERM;
	}

	if (--mutex->counter == 0) {
		if (unlikely(!atomic_cas(target, current_value, 0))) {
			/* In the unlikely event that a waiter was added after our read,
			 * try again with the waiter bit
			 */
			current_value |= HAS_WAITER;
			if (unlikely(!atomic_cas(target, current_value, 0))) {
				/* Should never happen */
				return -EFAULT;
			}
		}

		if (current_value & HAS_WAITER) {
			ret = k_futex_wake(&mutex->futex, false) >= 0 ? 0 : -EINVAL;
		}
	}

	return ret;
}
