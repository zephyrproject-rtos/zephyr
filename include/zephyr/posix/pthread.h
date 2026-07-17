/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Threads.
 * @ingroup posix
 *
 * Covers thread lifecycle, attributes, cancellation, scheduling, mutexes,
 * condition variables, barriers, reader-writer locks, spin locks, and
 * thread-specific data.
 *
 * @posix_header{pthread.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_PTHREAD_H_
#define ZEPHYR_INCLUDE_POSIX_PTHREAD_H_

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sched.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Pthread detach/joinable
 * Undefine possibly predefined values by external toolchain headers
 */
#undef PTHREAD_CREATE_DETACHED

/**
 * @brief Thread is created in the detached state.
 */
#define PTHREAD_CREATE_DETACHED 1
#undef PTHREAD_CREATE_JOINABLE

/**
 * @brief Thread is created in the joinable state (default).
 */
#define PTHREAD_CREATE_JOINABLE 0

/* Pthread resource visibility */

/**
 * @brief Mutex or barrier attribute: object is private to the process (default).
 */
#define PTHREAD_PROCESS_PRIVATE 0

/**
 * @brief Mutex or barrier attribute: object is shared between processes.
 */
#define PTHREAD_PROCESS_SHARED  1

/* Pthread cancellation */

/**
 * @brief Value returned by pthread_join() for a canceled thread.
 */
#define PTHREAD_CANCELED       ((void *)-1)

/**
 * @brief Cancellation is enabled (default).
 */
#define PTHREAD_CANCEL_ENABLE  0

/**
 * @brief Cancellation delivery is disabled.
 */
#define PTHREAD_CANCEL_DISABLE 1

/**
 * @brief Cancellation is deferred until a cancellation point (default).
 */
#define PTHREAD_CANCEL_DEFERRED     0

/**
 * @brief Thread cancellation is performed immediately.
 */
#define PTHREAD_CANCEL_ASYNCHRONOUS 1

/* Pthread scope */
#undef PTHREAD_SCOPE_PROCESS

/**
 * @brief Thread competes for resources only with threads in the same process.
 */
#define PTHREAD_SCOPE_PROCESS    1
#undef PTHREAD_SCOPE_SYSTEM

/**
 * @brief Thread competes for resources with all threads in the system (default).
 */
#define PTHREAD_SCOPE_SYSTEM     0

/* Pthread inherit scheduler */
#undef PTHREAD_INHERIT_SCHED

/**
 * @brief Thread inherits the scheduling policy of its creator (default).
 */
#define PTHREAD_INHERIT_SCHED  0
#undef PTHREAD_EXPLICIT_SCHED

/**
 * @brief Thread uses an explicitly provided scheduling policy.
 */
#define PTHREAD_EXPLICIT_SCHED 1

/* Passed to pthread_once */

/**
 * @brief Initializer for one-time initialization control.
 */
#define PTHREAD_ONCE_INIT {0}

/* The minimum allowable stack size */

/**
 * @brief Minimum thread stack size.
 */
#define PTHREAD_STACK_MIN K_KERNEL_STACK_LEN(0)

/**
 * @brief Declare a condition variable as initialized
 *
 * Initialize a condition variable with the default condition variable attributes.
 */
#define PTHREAD_COND_INITIALIZER (-1)

/**
 * @brief Initialize a condition variable.
 *
 * @param cv  Condition variable to initialize.
 * @param att Condition variable attributes, or NULL for defaults.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_cond_init}
 */
int pthread_cond_init(pthread_cond_t *cv, const pthread_condattr_t *att);

/**
 * @brief Destroy a condition variable.
 *
 * @param cv Condition variable to destroy.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_cond_destroy}
 */
int pthread_cond_destroy(pthread_cond_t *cv);

/**
 * @brief Signal a condition variable, waking at least one waiting thread.
 *
 * @param cv Condition variable to signal.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_cond_signal}
 */
int pthread_cond_signal(pthread_cond_t *cv);

/**
 * @brief Broadcast a condition variable, waking all waiting threads.
 *
 * @param cv Condition variable to broadcast.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_cond_broadcast}
 */
int pthread_cond_broadcast(pthread_cond_t *cv);

/**
 * @brief Wait on a condition variable.
 *
 * @param cv  Condition variable.
 * @param mut Associated mutex (must be locked by the caller).
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_cond_wait}
 */
int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *mut);

/**
 * @brief Wait on a condition variable with an absolute timeout.
 *
 * @param cv      Condition variable.
 * @param mut     Associated mutex (must be locked by the caller).
 * @param abstime Absolute timeout (@c CLOCK_REALTIME).
 *
 * @return 0 on success, @c ETIMEDOUT on timeout, or a positive error number on failure.
 *
 * @posix_func{pthread_cond_timedwait}
 */
int pthread_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *mut,
			   const struct timespec *abstime);

/**
 * @brief Initialize a condition variable attributes object with default values.
 *
 * @param att Condition variable attributes object to initialize.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_condattr_init}
 */
int pthread_condattr_init(pthread_condattr_t *att);

/**
 * @brief Destroy a condition variable attributes object.
 *
 * @param att Condition variable attributes object to destroy.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_condattr_destroy}
 */
int pthread_condattr_destroy(pthread_condattr_t *att);

/**
 * @brief Get the clock attribute of a condition variable attributes object.
 *
 * @param att      Condition variable attributes object.
 * @param clock_id Output: clock ID.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_condattr_getclock}
 */
int pthread_condattr_getclock(const pthread_condattr_t *ZRESTRICT att,
		clockid_t *ZRESTRICT clock_id);

/**
 * @brief Set the clock attribute of a condition variable attributes object.
 *
 * @param att      Condition variable attributes object.
 * @param clock_id Clock ID (e.g. @c CLOCK_MONOTONIC or @c CLOCK_REALTIME).
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_condattr_setclock}
 */

int pthread_condattr_setclock(pthread_condattr_t *att, clockid_t clock_id);

/**
 * @brief Declare a mutex as initialized
 *
 * Initialize a mutex with the default mutex attributes.
 */
#define PTHREAD_MUTEX_INITIALIZER (-1)

/**
 * @brief Declare a rwlock as initialized
 *
 * Initialize a rwlock with the default rwlock attributes.
 */
#define PTHREAD_RWLOCK_INITIALIZER (-1)

/*
 *  Mutex attributes - type
 *
 *  PTHREAD_MUTEX_NORMAL: Owner of mutex cannot relock it. Attempting
 *      to relock will cause deadlock.
 *  PTHREAD_MUTEX_RECURSIVE: Owner can relock the mutex.
 *  PTHREAD_MUTEX_ERRORCHECK: If owner attempts to relock the mutex, an
 *      error is returned.
 *
 */

/**
 * @brief Non-recursive mutex with no error checks (fastest).
 */
#define PTHREAD_MUTEX_NORMAL        0

/**
 * @brief Recursive mutex; the owning thread may lock it multiple times.
 */
#define PTHREAD_MUTEX_RECURSIVE     1

/**
 * @brief Mutex that returns an error on deadlock or unlock by a non-owner.
 */
#define PTHREAD_MUTEX_ERRORCHECK    2

/**
 * @brief Default mutex type; behavior on deadlock or unlock by a non-owner is undefined.
 */
#define PTHREAD_MUTEX_DEFAULT       PTHREAD_MUTEX_NORMAL

/*
 *  Mutex attributes - protocol
 *
 *  PTHREAD_PRIO_NONE: Ownership of mutex does not affect priority.
 *  PTHREAD_PRIO_INHERIT: Owner's priority is boosted to the priority of
 *      highest priority thread blocked on the mutex.
 *  PTHREAD_PRIO_PROTECT:  Mutex has a priority ceiling.  The owner's
 *      priority is boosted to the highest priority ceiling of all mutexes
 *      owned (regardless of whether or not other threads are blocked on
 *      any of these mutexes).
 *  FIXME: Only PRIO_NONE is supported. Implement other protocols.
 */

/**
 * @brief Mutex protocol: no priority inheritance or ceiling.
 */
#define PTHREAD_PRIO_NONE           0

/**
 * @brief Mutex protocol: owning thread inherits the priority of the highest-priority waiter.
 */
#define PTHREAD_PRIO_INHERIT        1

/**
 * @brief Mutex protocol: owner runs at ceiling priority while holding the mutex.
 */
#define PTHREAD_PRIO_PROTECT        2

/**
 * @brief Destroy a mutex.
 *
 * @param m Mutex to destroy.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutex_destroy}
 */
int pthread_mutex_destroy(pthread_mutex_t *m);

/**
 * @brief Lock a mutex, blocking until it is available.
 *
 * @param m Mutex to lock.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutex_lock}
 */
int pthread_mutex_lock(pthread_mutex_t *m);

/**
 * @brief Unlock a mutex.
 *
 * @param m Mutex to unlock.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutex_unlock}
 */
int pthread_mutex_unlock(pthread_mutex_t *m);

/**
 * @brief Lock a mutex with an absolute timeout.
 *
 * @param m       Mutex to lock.
 * @param abstime Absolute timeout (@c CLOCK_REALTIME).
 *
 * @return 0 on success, @c ETIMEDOUT on timeout, or a positive error number on failure.
 *
 * @posix_func{pthread_mutex_timedlock}
 */

int pthread_mutex_timedlock(pthread_mutex_t *m,
			    const struct timespec *abstime);

/**
 * @brief Try to lock a mutex without blocking.
 *
 * @param m Mutex to try to lock.
 *
 * @return 0 if the lock was acquired, @c EBUSY if already locked, or a positive error number.
 *
 * @posix_func{pthread_mutex_trylock}
 */
int pthread_mutex_trylock(pthread_mutex_t *m);

/**
 * @brief Initialize a mutex.
 *
 * @param m   Mutex to initialize.
 * @param att Mutex attributes, or NULL for defaults.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutex_init}
 */
int pthread_mutex_init(pthread_mutex_t *m,
				     const pthread_mutexattr_t *att);

/**
 * @brief Set the protocol attribute of a mutex attributes object.
 *
 * @param attr     Mutex attributes object.
 * @param protocol @c PTHREAD_PRIO_NONE, @c PTHREAD_PRIO_INHERIT, or @c PTHREAD_PRIO_PROTECT.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutexattr_setprotocol}
 */
int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol);

/**
 * @brief Set the type attribute of a mutex attributes object.
 *
 * @param attr Mutex attributes object.
 * @param type @c PTHREAD_MUTEX_NORMAL, @c PTHREAD_MUTEX_ERRORCHECK,
 *             @c PTHREAD_MUTEX_RECURSIVE, or @c PTHREAD_MUTEX_DEFAULT.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutexattr_settype}
 */
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);

/**
 * @brief Get the protocol attribute of a mutex attributes object.
 *
 * @param attr     Mutex attributes object.
 * @param protocol Output: @c PTHREAD_PRIO_NONE, @c PTHREAD_PRIO_INHERIT,
 *                 or @c PTHREAD_PRIO_PROTECT.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutexattr_getprotocol}
 */
int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr,
				  int *protocol);

/**
 * @brief Get the type attribute of a mutex attributes object.
 *
 * @param attr Mutex attributes object.
 * @param type Output: @c PTHREAD_MUTEX_NORMAL, @c PTHREAD_MUTEX_ERRORCHECK,
 *             @c PTHREAD_MUTEX_RECURSIVE, or @c PTHREAD_MUTEX_DEFAULT.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutexattr_gettype}
 */
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);

/**
 * @brief Initialize a mutex attributes object with default values.
 *
 * Note that pthread attribute structs are currently noops in Zephyr.
 *
 * @param attr Mutex attributes object to initialize.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutexattr_init}
 */
int pthread_mutexattr_init(pthread_mutexattr_t *attr);

/**
 * @brief Destroy a mutex attributes object.
 *
 * Note that pthread attribute structs are currently noops in Zephyr.
 *
 * @param attr Mutex attributes object to destroy.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutexattr_destroy}
 */
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

/**
 * @brief Returned by pthread_barrier_wait() to one (arbitrary) thread per barrier cycle.
 */
#define PTHREAD_BARRIER_SERIAL_THREAD 1

/*
 *  Barrier attributes - type
 */

/**
 * @brief Mutex or barrier attribute: object is private to the process (default).
 */
#define PTHREAD_PROCESS_PRIVATE		0

/**
 * @brief Non-standard alias for @c PTHREAD_PROCESS_SHARED.
 */
#define PTHREAD_PROCESS_PUBLIC		1

/**
 * @brief Synchronize participating threads at a barrier.
 *
 * Blocks until the number of threads specified in pthread_barrier_init() have
 * called this function on the same barrier. Exactly one thread receives
 * @c PTHREAD_BARRIER_SERIAL_THREAD as the return value.
 *
 * @param b Barrier to wait on.
 *
 * @return @c PTHREAD_BARRIER_SERIAL_THREAD for one thread, 0 for all others,
 *         or a positive error number on failure.
 *
 * @posix_func{pthread_barrier_wait}
 */
int pthread_barrier_wait(pthread_barrier_t *b);

/**
 * @brief Initialize a barrier object.
 *
 * @param b     Barrier to initialize.
 * @param attr  Barrier attributes, or NULL for defaults.
 * @param count Number of threads that must call pthread_barrier_wait() before any proceeds.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_barrier_init}
 */
int pthread_barrier_init(pthread_barrier_t *b, const pthread_barrierattr_t *attr,
			 unsigned int count);

/**
 * @brief Destroy a barrier object.
 *
 * @param b Barrier to destroy.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_barrier_destroy}
 */
int pthread_barrier_destroy(pthread_barrier_t *b);

/**
 * @brief Initialize a barrier attributes object with default values.
 *
 * @param b Barrier attributes object to initialize.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_barrierattr_init}
 */
int pthread_barrierattr_init(pthread_barrierattr_t *b);

/**
 * @brief Destroy a barrier attributes object.
 *
 * @param b Barrier attributes object to destroy.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_barrierattr_destroy}
 */
int pthread_barrierattr_destroy(pthread_barrierattr_t *b);

/**
 * @brief Set the process-shared attribute of a barrier attributes object.
 *
 * @param attr    Barrier attributes object.
 * @param pshared @c PTHREAD_PROCESS_SHARED or @c PTHREAD_PROCESS_PRIVATE.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_barrierattr_setpshared}
 */
int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr, int pshared);

/**
 * @brief Get the process-shared attribute of a barrier attributes object.
 *
 * @param attr    Barrier attributes object.
 * @param pshared Output: @c PTHREAD_PROCESS_SHARED or @c PTHREAD_PROCESS_PRIVATE.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_barrierattr_getpshared}
 */
int pthread_barrierattr_getpshared(const pthread_barrierattr_t *ZRESTRICT attr,
				   int *ZRESTRICT pshared);

/* Predicates and setters for various pthread attribute values that we
 * don't support (or always support: the "process shared" attribute
 * can only be true given the way Zephyr implements these
 * objects). Leave these undefined for simplicity instead of defining
 * stubs to return an error that would have to be logged and
 * interpreted just to figure out that we didn't support it in the
 * first place. These APIs are very rarely used even in production
 * Unix code.  Leave the declarations here so they can be easily
 * uncommented and implemented as needed.

int pthread_condattr_getpshared(const pthread_condattr_t * int *);
int pthread_condattr_setpshared(pthread_condattr_t *, int);
int pthread_mutex_consistent(pthread_mutex_t *);
int pthread_mutexattr_getpshared(const pthread_mutexattr_t * int *);
int pthread_mutexattr_getrobust(const pthread_mutexattr_t * int *);
int pthread_mutexattr_setpshared(pthread_mutexattr_t *, int);
int pthread_mutexattr_setrobust(pthread_mutexattr_t *, int);
*/

#ifdef CONFIG_POSIX_THREAD_PRIO_PROTECT
/**
 * @brief Get the priority ceiling of a mutex.
 *
 * @param mutex       Mutex to query.
 * @param prioceiling Output: priority ceiling value.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutex_getprioceiling}
 */
int pthread_mutex_getprioceiling(const pthread_mutex_t *ZRESTRICT mutex,
				 int *ZRESTRICT prioceiling);

/**
 * @brief Set the priority ceiling of a mutex.
 *
 * @param mutex       Mutex to update.
 * @param prioceiling New priority ceiling value.
 * @param old_ceiling Output: previous priority ceiling.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutex_setprioceiling}
 */
int pthread_mutex_setprioceiling(pthread_mutex_t *ZRESTRICT mutex, int prioceiling,
				 int *ZRESTRICT old_ceiling);

/**
 * @brief Get the priority ceiling attribute of a mutex attributes object.
 *
 * @param attr        Mutex attributes object.
 * @param prioceiling Output: priority ceiling.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutexattr_getprioceiling}
 */
int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *ZRESTRICT attr,
				     int *ZRESTRICT prioceiling);

/**
 * @brief Set the priority ceiling attribute of a mutex attributes object.
 *
 * @param attr        Mutex attributes object.
 * @param prioceiling Priority ceiling value.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_mutexattr_setprioceiling}
 */
int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, int prioceiling);
#endif /* CONFIG_POSIX_THREAD_PRIO_PROTECT */

/* Base Pthread related APIs */

/**
 * @brief Return the thread ID of the calling thread.
 *
 * The results of calling this API from threads not created with
 * pthread_create() are undefined.
 *
 * @return Thread ID of the calling thread.
 *
 * @posix_func{pthread_self}
 */
pthread_t pthread_self(void);

/**
 * @brief Compare two thread IDs.
 *
 * @param pt1 First thread ID.
 * @param pt2 Second thread ID.
 *
 * @return Non-zero if @p pt1 and @p pt2 refer to the same thread, 0 otherwise.
 *
 * @posix_func{pthread_equal}
 */
int pthread_equal(pthread_t pt1, pthread_t pt2);

/**
 * @brief Destroy a reader-writer lock attributes object.
 *
 * @param attr Attributes object to destroy.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlockattr_destroy}
 */
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);

/**
 * @brief Initialize a reader-writer lock attributes object with default values.
 *
 * @param attr Attributes object to initialize.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlockattr_init}
 */
int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);

/**
 * @brief Get the process-shared attribute of a reader-writer lock attributes object.
 *
 * @param attr    Attributes object.
 * @param pshared Output: @c PTHREAD_PROCESS_SHARED or @c PTHREAD_PROCESS_PRIVATE.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlockattr_getpshared}
 */
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *ZRESTRICT attr,
				  int *ZRESTRICT pshared);

/**
 * @brief Set the process-shared attribute of a reader-writer lock attributes object.
 *
 * @param attr    Attributes object.
 * @param pshared @c PTHREAD_PROCESS_SHARED or @c PTHREAD_PROCESS_PRIVATE.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlockattr_setpshared}
 */
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared);

/**
 * @brief Get the guard size attribute of a thread attributes object.
 *
 * @param attr      Thread attributes object.
 * @param guardsize Output: guard size in bytes.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_getguardsize}
 */
int pthread_attr_getguardsize(const pthread_attr_t *ZRESTRICT attr, size_t *ZRESTRICT guardsize);

/**
 * @brief Get the stack size attribute of a thread attributes object.
 *
 * @param attr      Thread attributes object.
 * @param stacksize Output: stack size in bytes.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_getstacksize}
 */
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);

/**
 * @brief Set the guard size attribute of a thread attributes object.
 *
 * @param attr      Thread attributes object.
 * @param guardsize Guard size in bytes.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_setguardsize}
 */
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);

/**
 * @brief Set the stack size attribute of a thread attributes object.
 *
 * @param attr      Thread attributes object.
 * @param stacksize Stack size in bytes (at least @c PTHREAD_STACK_MIN).
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_setstacksize}
 */
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);

/**
 * @brief Set the scheduling policy attribute of a thread attributes object.
 *
 * @param attr   Thread attributes object.
 * @param policy Scheduling policy (e.g. @c SCHED_FIFO, @c SCHED_RR, or @c SCHED_OTHER).
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_setschedpolicy}
 */
int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);

/**
 * @brief Get the scheduling policy attribute of a thread attributes object.
 *
 * @param attr   Thread attributes object.
 * @param policy Output: scheduling policy (e.g. @c SCHED_FIFO, @c SCHED_RR, or @c SCHED_OTHER).
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_getschedpolicy}
 */
int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy);

/**
 * @brief Set the detach state attribute of a thread attributes object.
 *
 * @param attr        Thread attributes object.
 * @param detachstate @c PTHREAD_CREATE_DETACHED or @c PTHREAD_CREATE_JOINABLE.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_setdetachstate}
 */
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);

/**
 * @brief Get the detach state attribute of a thread attributes object.
 *
 * @param attr        Thread attributes object.
 * @param detachstate Output: @c PTHREAD_CREATE_DETACHED or @c PTHREAD_CREATE_JOINABLE.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_getdetachstate}
 */
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);

/**
 * @brief Initialize a thread attributes object with default values.
 *
 * @param attr Thread attributes object to initialize.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_init}
 */
int pthread_attr_init(pthread_attr_t *attr);

/**
 * @brief Destroy a thread attributes object.
 *
 * @param attr Thread attributes object to destroy.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_destroy}
 */
int pthread_attr_destroy(pthread_attr_t *attr);

/**
 * @brief Get the scheduling parameter attribute of a thread attributes object.
 *
 * @param attr       Thread attributes object.
 * @param schedparam Output: scheduling parameters.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_getschedparam}
 */
int pthread_attr_getschedparam(const pthread_attr_t *attr,
			       struct sched_param *schedparam);

/**
 * @brief Get the scheduling policy and parameters of a thread.
 *
 * @param pthread Thread to query.
 * @param policy  Output: scheduling policy.
 * @param param   Output: scheduling parameters.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_getschedparam}
 */
int pthread_getschedparam(pthread_t pthread, int *policy,
			  struct sched_param *param);

/**
 * @brief Get the stack address and size attributes of a thread attributes object.
 *
 * @param attr      Thread attributes object.
 * @param stackaddr Output: stack base address.
 * @param stacksize Output: stack size in bytes.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_getstack}
 */
int pthread_attr_getstack(const pthread_attr_t *attr,
			  void **stackaddr, size_t *stacksize);

/**
 * @brief Set the stack address and size attributes of a thread attributes object.
 *
 * @param attr      Thread attributes object.
 * @param stackaddr Stack base address.
 * @param stacksize Stack size in bytes (at least @c PTHREAD_STACK_MIN).
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_setstack}
 */
int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr,
			  size_t stacksize);

/**
 * @brief Get the contention scope attribute of a thread attributes object.
 *
 * @param attr            Thread attributes object.
 * @param contentionscope Output: @c PTHREAD_SCOPE_SYSTEM or @c PTHREAD_SCOPE_PROCESS.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_getscope}
 */
int pthread_attr_getscope(const pthread_attr_t *attr, int *contentionscope);

/**
 * @brief Set the contention scope attribute of a thread attributes object.
 *
 * @param attr            Thread attributes object.
 * @param contentionscope @c PTHREAD_SCOPE_SYSTEM or @c PTHREAD_SCOPE_PROCESS.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_setscope}
 */
int pthread_attr_setscope(pthread_attr_t *attr, int contentionscope);

/**
 * @brief Get the inherit-scheduler attribute of a thread attributes object.
 *
 * @param attr         Thread attributes object.
 * @param inheritsched Output: @c PTHREAD_INHERIT_SCHED or @c PTHREAD_EXPLICIT_SCHED.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_getinheritsched}
 */
int pthread_attr_getinheritsched(const pthread_attr_t *attr, int *inheritsched);

/**
 * @brief Set the inherit-scheduler attribute of a thread attributes object.
 *
 * @param attr         Thread attributes object.
 * @param inheritsched @c PTHREAD_INHERIT_SCHED or @c PTHREAD_EXPLICIT_SCHED.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_setinheritsched}
 */
int pthread_attr_setinheritsched(pthread_attr_t *attr, int inheritsched);
#ifdef CONFIG_POSIX_THREADS
/**
 * @brief Ensure a one-time initialization routine is called exactly once.
 *
 * @param once     Pointer to a @c pthread_once_t initialized with @c PTHREAD_ONCE_INIT.
 * @param initFunc Initialization function to call at most once.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_once}
 */
int pthread_once(pthread_once_t *once, void (*initFunc)(void));
#endif

/**
 * @brief Terminate the calling thread.
 *
 * @param retval Value made available to pthread_join().
 *
 * @posix_func{pthread_exit}
 */
FUNC_NORETURN void pthread_exit(void *retval);

/**
 * @brief Join a thread with a non-portable timeout.
 * @ingroup posix_extensions
 *
 * @param thread  Thread to wait for.
 * @param status  Output: exit value of the thread, or @c PTHREAD_CANCELED.
 * @param abstime Absolute timeout (@c CLOCK_REALTIME).
 *
 * @return 0 on success, @c ETIMEDOUT on timeout, or a positive error number on failure.
 *
 * @compat_api{pthread_timedjoin_np}
 */
int pthread_timedjoin_np(pthread_t thread, void **status, const struct timespec *abstime);

/**
 * @brief Try to join a thread without blocking.
 * @ingroup posix_extensions
 *
 * @param thread Thread to try to join.
 * @param status Output: exit value if joined, or unchanged if not yet terminated.
 *
 * @return 0 if joined, @c EBUSY if the thread has not yet terminated,
 *         or a positive error number on failure.
 *
 * @compat_api{pthread_tryjoin_np}
 */
int pthread_tryjoin_np(pthread_t thread, void **status);

/**
 * @brief Wait for a thread to terminate and retrieve its exit status.
 *
 * @param thread Thread to wait for.
 * @param status Output: exit value of the thread, or @c PTHREAD_CANCELED.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_join}
 */
int pthread_join(pthread_t thread, void **status);

/**
 * @brief Send a cancellation request to a thread.
 *
 * @param pthread Thread to cancel.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_cancel}
 */
int pthread_cancel(pthread_t pthread);

/**
 * @brief Detach a thread, releasing its resources automatically on termination.
 *
 * @param thread Thread to detach.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_detach}
 */
int pthread_detach(pthread_t thread);

/**
 * @brief Create a new thread.
 *
 * @param newthread     Output: thread ID of the new thread.
 * @param attr          Thread attributes, or NULL for defaults.
 * @param threadroutine Thread entry point function.
 * @param arg           Argument passed to @p threadroutine.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_create}
 */
int pthread_create(pthread_t *newthread, const pthread_attr_t *attr,
		   void *(*threadroutine)(void *), void *arg);

/**
 * @brief Set the cancelability state of the calling thread.
 *
 * @param state    @c PTHREAD_CANCEL_ENABLE or @c PTHREAD_CANCEL_DISABLE.
 * @param oldstate Output: previous cancelability state.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_setcancelstate}
 */
int pthread_setcancelstate(int state, int *oldstate);

/**
 * @brief Set the cancelability type of the calling thread.
 *
 * @param type    @c PTHREAD_CANCEL_DEFERRED or @c PTHREAD_CANCEL_ASYNCHRONOUS.
 * @param oldtype Output: previous cancelability type.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_setcanceltype}
 */
int pthread_setcanceltype(int type, int *oldtype);

/**
 * @brief Create a cancellation point in the calling thread.
 *
 * If cancellation is pending and enabled, this function does not return.
 *
 * @posix_func{pthread_testcancel}
 */
void pthread_testcancel(void);

/**
 * @brief Set the scheduling parameter attribute of a thread attributes object.
 *
 * @param attr       Thread attributes object.
 * @param schedparam Scheduling parameters to apply.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_attr_setschedparam}
 */
int pthread_attr_setschedparam(pthread_attr_t *attr,
			       const struct sched_param *schedparam);

/**
 * @brief Set the scheduling policy and parameters of a thread.
 *
 * @param pthread Thread to modify.
 * @param policy  New scheduling policy.
 * @param param   New scheduling parameters.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_setschedparam}
 */
int pthread_setschedparam(pthread_t pthread, int policy,
			  const struct sched_param *param);

/**
 * @brief Set the scheduling priority of a thread.
 *
 * @param thread Thread to modify.
 * @param prio   New priority value.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_setschedprio}
 */
int pthread_setschedprio(pthread_t thread, int prio);

/**
 * @brief Destroy a reader-writer lock.
 *
 * @param rwlock Reader-writer lock to destroy.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlock_destroy}
 */
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

/**
 * @brief Initialize a reader-writer lock.
 *
 * @param rwlock Reader-writer lock to initialize.
 * @param attr   Attributes, or NULL for defaults.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlock_init}
 */
int pthread_rwlock_init(pthread_rwlock_t *rwlock,
			const pthread_rwlockattr_t *attr);

/**
 * @brief Acquire a read lock, blocking until available.
 *
 * @param rwlock Reader-writer lock.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlock_rdlock}
 */
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);

/**
 * @brief Acquire a read lock with an absolute timeout.
 *
 * @param rwlock  Reader-writer lock.
 * @param abstime Absolute timeout (@c CLOCK_REALTIME).
 *
 * @return 0 on success, @c ETIMEDOUT on timeout, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlock_timedrdlock}
 */
int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock,
			       const struct timespec *abstime);

/**
 * @brief Acquire a write lock with an absolute timeout.
 *
 * @param rwlock  Reader-writer lock.
 * @param abstime Absolute timeout (@c CLOCK_REALTIME).
 *
 * @return 0 on success, @c ETIMEDOUT on timeout, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlock_timedwrlock}
 */
int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock,
			       const struct timespec *abstime);

/**
 * @brief Try to acquire a read lock without blocking.
 *
 * @param rwlock Reader-writer lock.
 *
 * @return 0 on success, @c EBUSY if locked, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlock_tryrdlock}
 */
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);

/**
 * @brief Try to acquire a write lock without blocking.
 *
 * @param rwlock Reader-writer lock.
 *
 * @return 0 on success, @c EBUSY if locked, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlock_trywrlock}
 */
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);

/**
 * @brief Release a reader-writer lock.
 *
 * @param rwlock Reader-writer lock to release.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlock_unlock}
 */
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

/**
 * @brief Acquire a write lock, blocking until available.
 *
 * @param rwlock Reader-writer lock.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_rwlock_wrlock}
 */
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);

/**
 * @brief Create a thread-specific data key.
 *
 * @param key        Output: new thread-specific data key.
 * @param destructor Destructor called with the thread's value when the thread exits, or NULL.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_key_create}
 */
int pthread_key_create(pthread_key_t *key,
		void (*destructor)(void *));

/**
 * @brief Delete a thread-specific data key.
 *
 * @param key Key to delete.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_key_delete}
 */
int pthread_key_delete(pthread_key_t key);

/**
 * @brief Set the thread-specific value associated with a key.
 *
 * @param key   Thread-specific data key.
 * @param value Value to associate with @p key for the calling thread.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_setspecific}
 */
int pthread_setspecific(pthread_key_t key, const void *value);

/**
 * @brief Get the thread-specific value associated with a key.
 *
 * @param key Thread-specific data key.
 *
 * @return Value associated with @p key for the calling thread, or NULL if none.
 *
 * @posix_func{pthread_getspecific}
 */
void *pthread_getspecific(pthread_key_t key);

/**
 * @brief Register fork handlers to be called around fork().
 *
 * @param prepare Handler called in the parent before fork().
 * @param parent  Handler called in the parent after fork().
 * @param child   Handler called in the child after fork().
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_atfork}
 */
int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));

/**
 * @brief Get the concurrency level (advisory, has no effect on scheduling).
 *
 * @return Current concurrency level.
 *
 * @posix_func{pthread_getconcurrency}
 */
int pthread_getconcurrency(void);

/**
 * @brief Set the concurrency level (advisory, has no effect on scheduling).
 *
 * @param new_level Desired concurrency level (0 = implementation default).
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_setconcurrency}
 */
int pthread_setconcurrency(int new_level);
/** @cond INTERNAL_HIDDEN */
void __z_pthread_cleanup_push(void *cleanup[3], void (*routine)(void *arg), void *arg);
/** @endcond */
/** @cond INTERNAL_HIDDEN */
void __z_pthread_cleanup_pop(int execute);
/** @endcond */

/**
 * @brief Establish a cancellation cleanup handler.
 *
 * The handler @p _rtn is called with @p _arg when the thread exits or calls
 * pthread_cleanup_pop() with a non-zero argument.
 *
 * @note This macro must be paired with pthread_cleanup_pop() in the same
 *       lexical scope.
 */
#define pthread_cleanup_push(_rtn, _arg)                                                           \
	do /* enforce '{'-like behaviour */ {                                                      \
		void *_z_pthread_cleanup[3];                                                       \
		__z_pthread_cleanup_push(_z_pthread_cleanup, _rtn, _arg)

/**
 * @brief Remove a cancellation cleanup handler.
 *
 * @param _ex If non-zero, the handler is executed before being removed.
 *
 * @note This macro must be paired with pthread_cleanup_push() in the same
 *       lexical scope.
 */
#define pthread_cleanup_pop(_ex)                                                                   \
		__z_pthread_cleanup_pop(_ex);                                                      \
	} /* enforce '}'-like behaviour */ while (0)

/* Glibc / Oracle Extension Functions */

/**
 * @brief Set name of POSIX thread.
 *
 * Non-portable, extension function that conforms with most
 * other definitions of this function.
 *
 * @param thread POSIX thread to set name
 * @param name Name string
 * @retval 0 Success
 * @retval ESRCH Thread does not exist
 * @retval EINVAL Name buffer is NULL
 * @retval <0 Negative value if kernel function error
 *
 * @ingroup posix_extensions
 * @compat_api{pthread_setname_np}
 */
int pthread_setname_np(pthread_t thread, const char *name);

/**
 * @brief Get name of POSIX thread and store in name buffer that is of size len.
 *
 * Non-portable, extension function that conforms with most
 * other definitions of this function.
 *
 * @param thread POSIX thread to obtain name information
 * @param name Destination buffer
 * @param len Destination buffer size
 * @retval 0 Success
 * @retval ESRCH Thread does not exist
 * @retval EINVAL Name buffer is NULL
 * @retval <0 negative value if kernel function error
 * @ingroup posix_extensions
 * @compat_api{pthread_getname_np}
 */
int pthread_getname_np(pthread_t thread, char *name, size_t len);

#ifdef CONFIG_POSIX_THREADS

/**
 * @brief Destroy a spin lock.
 *
 * @param lock Spin lock to destroy.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_spin_destroy}
 */
int pthread_spin_destroy(pthread_spinlock_t *lock);

/**
 * @brief Initialize a spin lock.
 *
 * @param lock    Spin lock to initialize.
 * @param pshared @c PTHREAD_PROCESS_SHARED or @c PTHREAD_PROCESS_PRIVATE.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_spin_init}
 */
int pthread_spin_init(pthread_spinlock_t *lock, int pshared);

/**
 * @brief Acquire a spin lock, busy-waiting until available.
 *
 * @param lock Spin lock to acquire.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_spin_lock}
 */
int pthread_spin_lock(pthread_spinlock_t *lock);

/**
 * @brief Try to acquire a spin lock without busy-waiting.
 *
 * @param lock Spin lock to try.
 *
 * @return 0 on success, @c EBUSY if already locked, or a positive error number on failure.
 *
 * @posix_func{pthread_spin_trylock}
 */
int pthread_spin_trylock(pthread_spinlock_t *lock);

/**
 * @brief Release a spin lock.
 *
 * @param lock Spin lock to release.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_spin_unlock}
 */
int pthread_spin_unlock(pthread_spinlock_t *lock);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_PTHREAD_H_ */
