/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <spinlock.h>
#include <kswap.h>
#include <wait_q.h>
#include <syscall_handler.h>
#include <init.h>
#include <ksched.h>

static struct k_spinlock lock;

static _wait_q_t *futex_find(atomic_t *addr)
{
	struct _k_object *obj;

	obj = z_object_find(addr);
	if (obj == NULL || obj->type != K_OBJ_SYS_MUTEX) {
		return NULL;
	}

	return (_wait_q_t *)obj->data;
}

static int futex_wake(atomic_t *addr, int val)
{
	k_spinlock_key_t key;
	_wait_q_t *thread_q;
	struct k_thread *thread;
	int woken = 0;

	key = k_spin_lock(&lock);

	thread_q = futex_find(addr);
	if (thread_q == NULL) {
		k_spin_unlock(&lock, key);
		return woken;
	}

	while (woken < val && (thread = z_unpend_first_thread(thread_q))) {
		z_ready_thread(thread);
		woken++;
	}

	z_reschedule(&lock, key);

	return woken;
}

static int futex_wait(atomic_t *addr, int val, s32_t timeout)
{
	k_spinlock_key_t key;
	_wait_q_t *thread_q;

	if (atomic_get(addr) != (atomic_val_t)val) {
		return -EAGAIN;
	}

	key = k_spin_lock(&lock);

	thread_q = futex_find(addr);
	if (thread_q == NULL) {
		k_spin_unlock(&lock, key);
		return -ENOMEM;
	}

	return z_pend_curr(&lock, key, thread_q, timeout);
}

int z_impl_k_futex(atomic_t *addr, int op, int val, s32_t timeout)
{
	switch ((enum futex_op)op) {
	case FUTEX_WAIT:
		return futex_wait(addr, val, timeout);
	case FUTEX_WAKE:
		return futex_wake(addr, val);
	default:
		return -EINVAL;
	}
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_futex, addr, op, val, timeout)
{
	Z_OOPS(Z_SYSCALL_VERIFY(addr != 0));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(addr, sizeof(atomic_val_t)));

	return z_impl_k_futex((atomic_t *)addr,
			(int)op, (int)val, (s32_t)timeout);
}
#endif
