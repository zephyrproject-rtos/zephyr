/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_MUTEX_H_
#define ZEPHYR_INCLUDE_SYS_MUTEX_H_

#include <stdint.h>

#include <zephyr/sys/atomic_types.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup user_mutex_apis User mode mutex APIs
 * @ingroup usermode_apis
 *
 * A sys_mutex lives in user memory. Uncontended lock and unlock operations are
 * atomic operations on the object itself; the kernel is entered only to block
 * on a contended mutex (k_futex_wait()) or to wake a waiter (k_futex_wake()).
 * Any thread that can write the object's memory can use it, so the trust
 * boundary is the memory domain rather than per-thread kernel object
 * permissions.
 *
 * A mutex created with @c K_MUTEX_PRIO_INHERIT is backed by a kernel mutex
 * that the kernel associates with the object's address. Every operation on it
 * is a system call, the kernel enforces ownership, and the owner inherits the
 * priority of the highest-priority waiter.
 * @{
 */

struct k_thread;

/** @brief Mutex that lives in user memory */
struct sys_mutex {
	/** @cond INTERNAL_HIDDEN */
	/* the leading words are a struct k_futex: lock word, kernel-maintained waiter count */
	atomic_t val;
	atomic_t waiters;
	struct k_thread *owner;
	uint16_t count;
	uint8_t options;
	/** @endcond */
};

/**
 * @brief Statically define and initialize a sys_mutex
 *
 * The mutex is recursive and does not use priority inheritance; see
 * @ref SYS_MUTEX_DEFINE_EXT for other configurations. It can be accessed
 * outside the module where it is defined using:
 *
 * @code extern struct sys_mutex <name>; @endcode
 *
 * Route this to memory domains using K_APP_DMEM().
 *
 * @param name Name of the mutex.
 */
#define SYS_MUTEX_DEFINE(name) struct sys_mutex name

/**
 * @brief Statically define and initialize a sys_mutex with options
 *
 * @param name Name of the mutex.
 * @param opts A `K_MUTEX_*` type, optionally combined with @c K_MUTEX_PRIO_INHERIT.
 */
#define SYS_MUTEX_DEFINE_EXT(name, opts) struct sys_mutex name = {.options = (opts)}

/**
 * @brief Initialize a mutex.
 *
 * This routine initializes a mutex object, prior to its first use. The mutex
 * is recursive and does not use priority inheritance.
 *
 * Upon completion, the mutex is available and does not have an owner.
 *
 * @param mutex Address of the mutex.
 */
void sys_mutex_init(struct sys_mutex *mutex);

/**
 * @brief Initialize a mutex with options.
 *
 * Upon completion, the mutex is available and does not have an owner.
 *
 * @param mutex Address of the mutex.
 * @param options A `K_MUTEX_*` type (@c K_MUTEX_RECURSIVE, @c K_MUTEX_NORMAL or
 *                @c K_MUTEX_ERRORCHECK), optionally combined with
 *                @c K_MUTEX_PRIO_INHERIT.
 *
 * @retval 0 Mutex initialized.
 * @retval -EINVAL Invalid options.
 * @retval -ENOTSUP Priority inheritance requested without
 *                  @kconfig{CONFIG_SYS_MUTEX_PRIO_INHERIT}.
 */
int sys_mutex_init_ext(struct sys_mutex *mutex, uint32_t options);

/**
 * @brief Destroy a mutex.
 *
 * Releases the kernel resources of a priority-inheritance mutex. The mutex
 * must be unlocked and must be initialized again before any further use.
 *
 * @param mutex Address of the mutex.
 *
 * @retval 0 Mutex destroyed.
 * @retval -EBUSY Mutex is locked.
 * @retval -EACCES Caller has no access to provided mutex address.
 */
int sys_mutex_destroy(struct sys_mutex *mutex);

/**
 * @brief Lock a mutex.
 *
 * This routine locks @a mutex. If the mutex is locked by another thread,
 * the calling thread waits until the mutex becomes available or until
 * a timeout occurs.
 *
 * A thread is permitted to lock a @c K_MUTEX_RECURSIVE mutex it has already
 * locked. The operation completes immediately and the lock count is increased
 * by 1. Relocking a @c K_MUTEX_ERRORCHECK mutex fails with -EDEADLK, and
 * relocking a @c K_MUTEX_NORMAL mutex waits like any other contended lock.
 *
 * @param mutex Address of the mutex, which may reside in user memory
 * @param timeout Waiting period to lock the mutex,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Mutex locked.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EDEADLK Caller already owns an error-checking mutex.
 * @retval -EACCES Caller has no access to provided mutex address
 * @retval -ENOMEM No kernel mutex is available to back a priority-inheritance mutex.
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
 * A @c K_MUTEX_NORMAL mutex without priority inheritance does not record its
 * owner, so it cannot detect an unlock by a thread other than the owner.
 *
 * @param mutex Address of the mutex, which may reside in user memory
 * @retval 0 Mutex unlocked
 * @retval -EACCES Caller has no access to provided mutex address
 * @retval -EINVAL Mutex wasn't locked
 * @retval -EPERM Caller does not own the mutex
 */
int sys_mutex_unlock(struct sys_mutex *mutex);

/** @cond INTERNAL_HIDDEN */
__syscall int z_sys_mutex_kernel_lock(struct sys_mutex *mutex, k_timeout_t timeout);

__syscall int z_sys_mutex_kernel_unlock(struct sys_mutex *mutex);

__syscall int z_sys_mutex_kernel_destroy(struct sys_mutex *mutex);
/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/mutex.h>

#endif /* ZEPHYR_INCLUDE_SYS_MUTEX_H_ */
