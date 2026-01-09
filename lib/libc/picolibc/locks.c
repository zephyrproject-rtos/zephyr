/*
 * Copyright © 2021, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "picolibc-hooks.h"

#ifdef CONFIG_MULTITHREADING
/* newlib expects `struct __lock` and `_LOCK_T` to be a pointer to it */
struct __lock {
#ifndef CONFIG_USERSPACE
	struct k_mutex mutex;
#else
	struct k_mutex *mutex;
#endif
};

typedef struct __lock * _LOCK_T;

struct __lock __lock___libc_recursive_mutex;

/* Initialise recursive locks and grant userspace access after boot */
static int picolibc_locks_prepare(void)
{
#ifndef CONFIG_USERSPACE
	k_mutex_init(&__lock___libc_recursive_mutex.mutex);
#else
	__lock___libc_recursive_mutex.mutex = k_object_alloc(K_OBJ_MUTEX);
	k_mutex_init(__lock___libc_recursive_mutex.mutex);
	k_object_access_all_grant(__lock___libc_recursive_mutex.mutex);
#endif

	return 0;
}

SYS_INIT(picolibc_locks_prepare, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* Create a new dynamic recursive lock */
void __retarget_lock_init_recursive(_LOCK_T *lock)
{
	__ASSERT_NO_MSG(lock != NULL);

	/* Allocate lock wrapper and underlying mutex as needed */
	struct __lock *obj = malloc(sizeof(struct __lock));
	__ASSERT(obj != NULL, "recursive lock allocation failed");

#ifndef CONFIG_USERSPACE
	k_mutex_init(&obj->mutex);
#else
	obj->mutex = k_object_alloc(K_OBJ_MUTEX);
	__ASSERT(obj->mutex != NULL, "k_object_alloc failed");
	k_mutex_init(obj->mutex);
#endif

	*lock = obj;
}

/* Create a new dynamic non-recursive lock */
void __retarget_lock_init(_LOCK_T *lock)
{
	__retarget_lock_init_recursive(lock);
}

/* Close dynamic recursive lock */
void __retarget_lock_close_recursive(_LOCK_T lock)
{
	__ASSERT_NO_MSG(lock != NULL);
#ifndef CONFIG_USERSPACE
	free(lock);
#else
	if (lock->mutex) {
		k_object_release(lock->mutex);
	}
	free(lock);
#endif /* !CONFIG_USERSPACE */
}

/* Close dynamic non-recursive lock */
void __retarget_lock_close(_LOCK_T lock)
{
	__retarget_lock_close_recursive(lock);
}

/* Acquiure recursive lock */
void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
	__ASSERT_NO_MSG(lock != NULL);
#ifndef CONFIG_USERSPACE
	k_mutex_lock(&lock->mutex, K_FOREVER);
#else
	k_mutex_lock(lock->mutex, K_FOREVER);
#endif
}

/* Acquiure non-recursive lock */
void __retarget_lock_acquire(_LOCK_T lock)
{
	__retarget_lock_acquire_recursive(lock);
}

/* Try acquiring recursive lock */
int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
	__ASSERT_NO_MSG(lock != NULL);
#ifndef CONFIG_USERSPACE
	return !k_mutex_lock(&lock->mutex, K_NO_WAIT);
#else
	return !k_mutex_lock(lock->mutex, K_NO_WAIT);
#endif
}

/* Try acquiring non-recursive lock */
int __retarget_lock_try_acquire(_LOCK_T lock)
{
	return __retarget_lock_try_acquire_recursive(lock);
}

/* Release recursive lock */
void __retarget_lock_release_recursive(_LOCK_T lock)
{
	__ASSERT_NO_MSG(lock != NULL);
#ifndef CONFIG_USERSPACE
	k_mutex_unlock(&lock->mutex);
#else
	k_mutex_unlock(lock->mutex);
#endif
}

/* Release non-recursive lock */
void __retarget_lock_release(_LOCK_T lock)
{
	__retarget_lock_release_recursive(lock);
}

#endif /* CONFIG_MULTITHREADING */
