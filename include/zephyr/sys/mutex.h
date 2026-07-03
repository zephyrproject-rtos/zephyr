/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_MUTEX_H_
#define ZEPHYR_INCLUDE_SYS_MUTEX_H_

/*
 * sys_mutex behaves almost exactly like k_mutex, with the added advantage
 * that a sys_mutex instance can reside in user memory.
 *
 * Depending on CONFIG_SYS_MUTEX_IMPL, sys_mutex is implemented either via
 * a backing k_mutex (preserving priority inheritance) or via atomics on top
 * of k_futex (allowing uncontended lock/unlock without syscalls on archs
 * where reading the current thread and atomic ops are both syscall-free).
 *
 * Further enhancements will support priority inheritance with atomics once
 * a corresponding futex is available.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_USERSPACE
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/types.h>
#include <zephyr/sys_clock.h>

/**
 * @brief Userspace mutex
 */
struct sys_mutex {
	/** @cond INTERNAL_HIDDEN */
#ifdef CONFIG_SYS_MUTEX_IMPL_K_MUTEX
	atomic_t val;
#elif defined(CONFIG_SYS_MUTEX_IMPL_FUTEX)
	struct k_futex futex;
	uint32_t counter;
#endif  /* CONFIG_SYS_MUTEX_IMPL_FUTEX */
	/** @endcond */
};

#ifdef CONFIG_SYS_MUTEX_IMPL_K_MUTEX
__syscall int z_sys_mutex_kernel_lock(struct sys_mutex *mutex, k_timeout_t timeout);

__syscall int z_sys_mutex_kernel_unlock(struct sys_mutex *mutex);

#include <zephyr/syscalls/mutex.h>
#endif /* CONFIG_SYS_MUTEX_IMPL_K_MUTEX */

/**
 * @defgroup user_mutex_apis User mode mutex APIs
 * @ingroup usermode_apis
 * @{
 */

/**
 * @brief Statically define and initialize a sys_mutex
 *
 * The mutex can be accessed outside the module where it is defined using:
 *
 * @code extern struct sys_mutex <name>; @endcode
 *
 * Route this to memory domains using K_APP_DMEM().
 *
 * @param name Name of the mutex.
 */
#if defined(CONFIG_SYS_MUTEX_IMPL_FUTEX)
#define SYS_MUTEX_DEFINE(name) struct sys_mutex name = {.futex = {0}, .counter = 0}
#else
#define SYS_MUTEX_DEFINE(name) struct sys_mutex name
#endif

/**
 * @brief Initialize a mutex.
 *
 * This routine initializes a mutex object, prior to its first use.
 *
 * Upon completion, the mutex is available and does not have an owner.
 *
 * This routine is only necessary to call when the mutex was not created
 * with SYS_MUTEX_DEFINE().
 *
 * @param mutex Address of the mutex.
 */
void sys_mutex_init(struct sys_mutex *mutex);

/**
 * @brief Lock a mutex.
 *
 * This routine locks @a mutex. If the mutex is locked by another thread,
 * the calling thread waits until the mutex becomes available or until
 * a timeout occurs.
 *
 * A thread is permitted to lock a mutex it has already locked. The operation
 * completes immediately and the lock count is increased by 1.
 *
 * @param mutex Address of the mutex, which may reside in user memory
 * @param timeout Waiting period to lock the mutex,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Mutex locked.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EACCES Caller has no access to provided mutex address
 * @retval -EINVAL Provided mutex not recognized by the kernel
 */
int sys_mutex_lock(struct sys_mutex *mutex, k_timeout_t timeout);

/**
 * @brief Unlock a mutex.
 *
 * This routine unlocks @a mutex. The mutex must already be locked by the
 * calling thread.
 *
 * The mutex cannot be claimed by another thread until it has been unlocked by
 * the calling thread as many times as it was previously locked by that
 * thread.
 *
 * @param mutex Address of the mutex, which may reside in user memory
 * @retval 0 Mutex unlocked
 * @retval -EACCES Caller has no access to provided mutex address
 * @retval -EINVAL Provided mutex not recognized by the kernel or mutex wasn't
 *                 locked
 * @retval -EPERM Caller does not own the mutex
 */
int sys_mutex_unlock(struct sys_mutex *mutex);

#else
#include <zephyr/kernel.h>

struct sys_mutex {
	struct k_mutex kernel_mutex;
};

#define SYS_MUTEX_DEFINE(name) \
	struct sys_mutex name = { \
		.kernel_mutex = Z_MUTEX_INITIALIZER(name.kernel_mutex) \
	}

static inline void sys_mutex_init(struct sys_mutex *mutex)
{
	k_mutex_init(&mutex->kernel_mutex);
}

static inline int sys_mutex_lock(struct sys_mutex *mutex, k_timeout_t timeout)
{
	return k_mutex_lock(&mutex->kernel_mutex, timeout);
}

static inline int sys_mutex_unlock(struct sys_mutex *mutex)
{
	return k_mutex_unlock(&mutex->kernel_mutex);
}

#endif /* CONFIG_USERSPACE */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_MUTEX_H_ */
