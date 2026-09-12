/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/spinlock.h>
#include <kswap.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/init.h>
#include <ksched.h>
#include <wait_q.h>

#define FUTEX_HASH_BUCKETS 16
BUILD_ASSERT(IS_POWER_OF_TWO(FUTEX_HASH_BUCKETS));

/*
 * Futexes at addresses the kernel object scanner cannot see (stack, heap, or
 * any other user-accessible memory) share this fixed set of address-hashed
 * wait queues. Waiters record which futex they wait on, and wake-ups unpend
 * only matching waiters, so sharing a bucket costs lock contention but never
 * a lost or spurious wakeup.
 */
#define FUTEX_HASH_BUCKET_INIT(i, _) Z_FUTEX_DATA_INITIALIZER(futex_hash_buckets[i])
static struct z_futex_data futex_hash_buckets[FUTEX_HASH_BUCKETS] = {
	LISTIFY(FUTEX_HASH_BUCKETS, FUTEX_HASH_BUCKET_INIT, (,))
};

#ifdef CONFIG_USERSPACE
static struct z_futex_data *futex_kobject_data(struct k_futex *futex)
{
	struct k_object *obj = k_object_find(futex);

	if ((obj != NULL) && (obj->type == K_OBJ_FUTEX)) {
		return obj->data.futex_data;
	}

	return NULL;
}
#else
static struct z_futex_data *futex_kobject_data(struct k_futex *futex)
{
	ARG_UNUSED(futex);

	return NULL;
}
#endif

static struct z_futex_data *k_futex_find_data(struct k_futex *futex)
{
	uintptr_t addr = (uintptr_t)futex;
	struct z_futex_data *futex_data = futex_kobject_data(futex);

	if (futex_data != NULL) {
		return futex_data;
	}

	return &futex_hash_buckets[(addr / sizeof(struct k_futex)) % FUTEX_HASH_BUCKETS];
}

int z_impl_k_futex_wake(struct k_futex *futex, bool wake_all)
{
	k_spinlock_key_t key;
	unsigned int woken = 0U;
	struct k_thread *thread;
	struct k_thread *iter;
	struct z_futex_data *futex_data;

	futex_data = k_futex_find_data(futex);

	key = k_spin_lock(&futex_data->lock);

	do {
		/* wake in priority order, skipping waiters that hashed to the same bucket */
		thread = NULL;
		_WAIT_Q_FOR_EACH(&futex_data->wait_q, iter) {
			if (iter->base.swap_data == (void *)futex) {
				thread = iter;
				break;
			}
		}

		if (thread != NULL) {
			z_unpend_thread(thread);
			(void)atomic_dec(&futex->waiters);
			woken++;
			arch_thread_return_value_set(thread, 0);
			z_ready_thread(thread);
		}
	} while (thread && wake_all);

	if (woken == 0) {
		k_spin_unlock(&futex_data->lock, key);
	} else {
		z_reschedule(&futex_data->lock, key);
	}

	return woken;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_futex_wake(struct k_futex *futex, bool wake_all)
{
	if (K_SYSCALL_MEMORY_WRITE(futex, sizeof(struct k_futex)) != 0) {
		return -EACCES;
	}

	return z_impl_k_futex_wake(futex, wake_all);
}
#include <zephyr/syscalls/k_futex_wake_mrsh.c>
#endif

int z_impl_k_futex_wait(struct k_futex *futex, int expected,
			k_timeout_t timeout)
{
	int ret;
	k_spinlock_key_t key;
	struct z_futex_data *futex_data;

	futex_data = k_futex_find_data(futex);

	key = k_spin_lock(&futex_data->lock);

	/* counted before the compare, so a waker that sees zero knows the value check follows */
	(void)atomic_inc(&futex->waiters);

	if (atomic_get(&futex->val) != (atomic_val_t)expected) {
		(void)atomic_dec(&futex->waiters);
		k_spin_unlock(&futex_data->lock, key);
		return -EAGAIN;
	}

	/* record which futex we wait on, so that wake-ups skip bucket-mates */
	_current->base.swap_data = futex;

	ret = z_pend_curr(&futex_data->lock,
			key, &futex_data->wait_q, timeout);
	if (ret == -EAGAIN) {
		/* timed out: nobody unpended us, so nobody decremented for us */
		(void)atomic_dec(&futex->waiters);
		ret = -ETIMEDOUT;
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_futex_wait(struct k_futex *futex, int expected,
				      k_timeout_t timeout)
{
	if (K_SYSCALL_MEMORY_WRITE(futex, sizeof(struct k_futex)) != 0) {
		return -EACCES;
	}

	return z_impl_k_futex_wait(futex, expected, timeout);
}
#include <zephyr/syscalls/k_futex_wait_mrsh.c>
#endif
