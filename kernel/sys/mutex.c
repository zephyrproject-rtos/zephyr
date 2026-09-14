/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/elastipool.h>
#include <zephyr/sys/hash_map.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/sys/util.h>

BUILD_ASSERT(offsetof(struct sys_mutex, val) == offsetof(struct k_futex, val));
BUILD_ASSERT(offsetof(struct sys_mutex, waiters) == offsetof(struct k_futex, waiters));
BUILD_ASSERT((K_MUTEX_PRIO_INHERIT & K_MUTEX_TYPE_MASK) == 0);
BUILD_ASSERT((K_MUTEX_TYPE_MASK | K_MUTEX_PRIO_INHERIT) <= UINT8_MAX);

#define SYS_MUTEX_UNLOCKED  0
#define SYS_MUTEX_LOCKED    1
#define SYS_MUTEX_CONTENDED 2

static inline struct k_futex *mutex_futex(struct sys_mutex *mutex)
{
	return (struct k_futex *)&mutex->val;
}

static inline bool mutex_is_pi(const struct sys_mutex *mutex)
{
	return (mutex->options & K_MUTEX_PRIO_INHERIT) != 0;
}

static inline uint32_t mutex_type(const struct sys_mutex *mutex)
{
	return K_MUTEX_TYPE(mutex->options);
}

void sys_mutex_init(struct sys_mutex *mutex)
{
	(void)sys_mutex_init_ext(mutex, K_MUTEX_RECURSIVE);
}

int sys_mutex_init_ext(struct sys_mutex *mutex, uint32_t options)
{
	switch (K_MUTEX_TYPE(options)) {
	case K_MUTEX_RECURSIVE:
	case K_MUTEX_NORMAL:
	case K_MUTEX_ERRORCHECK:
		break;
	default:
		return -EINVAL;
	}

	if ((options & ~(K_MUTEX_TYPE_MASK | K_MUTEX_PRIO_INHERIT)) != 0) {
		return -EINVAL;
	}

	if (((options & K_MUTEX_PRIO_INHERIT) != 0) &&
	    !IS_ENABLED(CONFIG_SYS_MUTEX_PRIO_INHERIT)) {
		return -ENOTSUP;
	}

	*mutex = (struct sys_mutex){.options = options};

	return 0;
}

int sys_mutex_destroy(struct sys_mutex *mutex)
{
	int ret;

	if (mutex_is_pi(mutex)) {
		ret = z_sys_mutex_kernel_destroy(mutex);
		if (ret < 0) {
			return ret;
		}
	} else if (atomic_get(&mutex->val) != SYS_MUTEX_UNLOCKED) {
		return -EBUSY;
	}

	*mutex = (struct sys_mutex){0};

	return 0;
}

int sys_mutex_lock(struct sys_mutex *mutex, k_timeout_t timeout)
{
	int ret;
	atomic_val_t val;
	k_timepoint_t end;
	k_tid_t self = NULL;

	if (mutex_is_pi(mutex)) {
		return z_sys_mutex_kernel_lock(mutex, timeout);
	}

	if (mutex_type(mutex) != K_MUTEX_NORMAL) {
		self = k_current_get();
		if ((atomic_get(&mutex->val) != SYS_MUTEX_UNLOCKED) && (mutex->owner == self)) {
			if (mutex_type(mutex) == K_MUTEX_ERRORCHECK) {
				return -EDEADLK;
			}
			if (mutex->count == UINT16_MAX) {
				return -EAGAIN;
			}
			mutex->count++;
			return 0;
		}
	}

	if (!atomic_cas(&mutex->val, SYS_MUTEX_UNLOCKED, SYS_MUTEX_LOCKED)) {
		if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
			return -EBUSY;
		}

		end = sys_timepoint_calc(timeout);
		val = atomic_set(&mutex->val, SYS_MUTEX_CONTENDED);
		while (val != SYS_MUTEX_UNLOCKED) {
			ret = k_futex_wait(mutex_futex(mutex), SYS_MUTEX_CONTENDED,
					   sys_timepoint_timeout(end));
			if (ret == -ETIMEDOUT) {
				return -EAGAIN;
			}
			if ((ret < 0) && (ret != -EAGAIN)) {
				return ret;
			}
			val = atomic_set(&mutex->val, SYS_MUTEX_CONTENDED);
		}
	}

	if (mutex_type(mutex) != K_MUTEX_NORMAL) {
		mutex->owner = self;
		mutex->count = 0;
	}

	return 0;
}

int sys_mutex_unlock(struct sys_mutex *mutex)
{
	if (mutex_is_pi(mutex)) {
		return z_sys_mutex_kernel_unlock(mutex);
	}

	if (atomic_get(&mutex->val) == SYS_MUTEX_UNLOCKED) {
		return -EINVAL;
	}

	if (mutex_type(mutex) != K_MUTEX_NORMAL) {
		if (mutex->owner != k_current_get()) {
			return -EPERM;
		}
		if (mutex->count > 0) {
			mutex->count--;
			return 0;
		}
		mutex->owner = NULL;
	}

	if (atomic_dec(&mutex->val) != SYS_MUTEX_LOCKED) {
		atomic_set(&mutex->val, SYS_MUTEX_UNLOCKED);
		(void)k_futex_wake(mutex_futex(mutex), false);
	}

	return 0;
}

#ifdef CONFIG_SYS_MUTEX_PRIO_INHERIT

BUILD_ASSERT(CONFIG_SYS_MUTEX_PRIO_INHERIT_MIN >= 0);
BUILD_ASSERT(CONFIG_SYS_MUTEX_PRIO_INHERIT_MAX >= CONFIG_SYS_MUTEX_PRIO_INHERIT_MIN);

/*
 * The kernel mutex behind a priority-inheritance sys_mutex is found by the
 * address of the user object, never through its contents. Kernel mutexes are
 * drawn from this pool on first lock and returned on destroy.
 */
#define PI_MUTEX_INIT(idx, name) Z_MUTEX_INITIALIZER(name[idx])
static struct k_mutex pi_mutex_storage[] = {
	LISTIFY(CONFIG_SYS_MUTEX_PRIO_INHERIT_MIN, PI_MUTEX_INIT, (,), pi_mutex_storage)};
SYS_ELASTIPOOL_DEFINE_ADVANCED(pi_mutex_pool, sizeof(struct k_mutex), __alignof(struct k_mutex),
			       CONFIG_SYS_MUTEX_PRIO_INHERIT_MIN, CONFIG_SYS_MUTEX_PRIO_INHERIT_MAX,
			       SYS_ELASTIPOOL_THREAD_HEAP_ALLOC_FN(CONFIG_SYS_MUTEX_PRIO_INHERIT_MIN,
								   CONFIG_SYS_MUTEX_PRIO_INHERIT_MAX),
			       pi_mutex_storage, static);
SYS_HASHMAP_DEFAULT_DEFINE_STATIC_ADVANCED(pi_mutex_map, sys_hash32, sys_elastipool_map_alloc,
					   SYS_HASHMAP_CONFIG(SIZE_MAX,
							      SYS_HASHMAP_DEFAULT_LOAD_FACTOR));
static struct k_spinlock pi_mutex_lock;

static struct k_mutex *pi_mutex_get(struct sys_mutex *mutex, bool create)
{
	int ret;
	uint64_t value;
	k_spinlock_key_t key;
	struct k_mutex *kmutex = NULL;

	key = k_spin_lock(&pi_mutex_lock);
	if (sys_hashmap_get(&pi_mutex_map, (uintptr_t)mutex, &value)) {
		kmutex = (struct k_mutex *)(uintptr_t)value;
	} else if (create && (sys_elastipool_alloc(&pi_mutex_pool, (void **)&kmutex) == 0)) {
		(void)z_impl_k_mutex_init_ext(kmutex, K_MUTEX_TYPE(mutex->options));
		ret = sys_hashmap_insert(&pi_mutex_map, (uintptr_t)mutex, (uintptr_t)kmutex, NULL);
		if (ret < 0) {
			(void)sys_elastipool_free(&pi_mutex_pool, kmutex);
			kmutex = NULL;
		}
	}
	k_spin_unlock(&pi_mutex_lock, key);

	return kmutex;
}

static void pi_mutex_put(struct sys_mutex *mutex, struct k_mutex *kmutex)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&pi_mutex_lock);
	(void)sys_hashmap_remove(&pi_mutex_map, (uintptr_t)mutex, NULL);
	(void)sys_elastipool_free(&pi_mutex_pool, kmutex);
	k_spin_unlock(&pi_mutex_lock, key);
}

int z_impl_z_sys_mutex_kernel_lock(struct sys_mutex *mutex, k_timeout_t timeout)
{
	struct k_mutex *kmutex = pi_mutex_get(mutex, true);

	if (kmutex == NULL) {
		return -ENOMEM;
	}

	return k_mutex_lock(kmutex, timeout);
}

int z_impl_z_sys_mutex_kernel_unlock(struct sys_mutex *mutex)
{
	struct k_mutex *kmutex = pi_mutex_get(mutex, false);

	if ((kmutex == NULL) || (kmutex->lock_count == 0U)) {
		return -EINVAL;
	}

	return k_mutex_unlock(kmutex);
}

int z_impl_z_sys_mutex_kernel_destroy(struct sys_mutex *mutex)
{
	struct k_mutex *kmutex = pi_mutex_get(mutex, false);

	if (kmutex == NULL) {
		return 0;
	}

	if (kmutex->lock_count != 0U) {
		return -EBUSY;
	}

	pi_mutex_put(mutex, kmutex);

	return 0;
}

#else /* CONFIG_SYS_MUTEX_PRIO_INHERIT */

int z_impl_z_sys_mutex_kernel_lock(struct sys_mutex *mutex, k_timeout_t timeout)
{
	ARG_UNUSED(mutex);
	ARG_UNUSED(timeout);

	return -ENOTSUP;
}

int z_impl_z_sys_mutex_kernel_unlock(struct sys_mutex *mutex)
{
	ARG_UNUSED(mutex);

	return -ENOTSUP;
}

int z_impl_z_sys_mutex_kernel_destroy(struct sys_mutex *mutex)
{
	ARG_UNUSED(mutex);

	return -ENOTSUP;
}

#endif /* CONFIG_SYS_MUTEX_PRIO_INHERIT */

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_z_sys_mutex_kernel_lock(struct sys_mutex *mutex, k_timeout_t timeout)
{
	if (K_SYSCALL_MEMORY_WRITE(mutex, sizeof(struct sys_mutex)) != 0) {
		return -EACCES;
	}

	return z_impl_z_sys_mutex_kernel_lock(mutex, timeout);
}
#include <zephyr/syscalls/z_sys_mutex_kernel_lock_mrsh.c>

static inline int z_vrfy_z_sys_mutex_kernel_unlock(struct sys_mutex *mutex)
{
	if (K_SYSCALL_MEMORY_WRITE(mutex, sizeof(struct sys_mutex)) != 0) {
		return -EACCES;
	}

	return z_impl_z_sys_mutex_kernel_unlock(mutex);
}
#include <zephyr/syscalls/z_sys_mutex_kernel_unlock_mrsh.c>

static inline int z_vrfy_z_sys_mutex_kernel_destroy(struct sys_mutex *mutex)
{
	if (K_SYSCALL_MEMORY_WRITE(mutex, sizeof(struct sys_mutex)) != 0) {
		return -EACCES;
	}

	return z_impl_z_sys_mutex_kernel_destroy(mutex);
}
#include <zephyr/syscalls/z_sys_mutex_kernel_destroy_mrsh.c>
#endif /* CONFIG_USERSPACE */
