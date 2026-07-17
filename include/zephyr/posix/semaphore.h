/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Semaphores.
 * @ingroup posix
 *
 * Provides counting semaphores for synchronization between threads and
 * processes, in both named (sem_open()) and unnamed (sem_init()) forms.
 *
 * @posix_header{semaphore.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SEMAPHORE_H_
#define ZEPHYR_INCLUDE_POSIX_SEMAPHORE_H_

#include <time.h>

#include <zephyr/posix/posix_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Value returned by sem_open() on failure.
 */
#define SEM_FAILED ((sem_t *) 0)

/**
 * @brief Destroy an unnamed semaphore.
 *
 * @param semaphore Semaphore to destroy.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sem_destroy}
 */
int sem_destroy(sem_t *semaphore);

/**
 * @brief Get the current value of a semaphore.
 *
 * @param semaphore Semaphore to query.
 * @param value     Output: current count (implementation may report 0 if waiters are present).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sem_getvalue}
 */
int sem_getvalue(sem_t *ZRESTRICT semaphore, int *ZRESTRICT value);

/**
 * @brief Initialize an unnamed semaphore.
 *
 * @param semaphore Semaphore to initialize.
 * @param pshared   Non-zero if the semaphore is shared between processes.
 * @param value     Initial count (must be <= @c SEM_VALUE_MAX).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sem_init}
 */
int sem_init(sem_t *semaphore, int pshared, unsigned int value);

/**
 * @brief Unlock a semaphore (increment its count).
 *
 * @param semaphore Semaphore to post.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sem_post}
 */
int sem_post(sem_t *semaphore);

/**
 * @brief Lock a semaphore with an absolute timeout.
 *
 * @param semaphore Semaphore to wait on.
 * @param abstime   Absolute timeout (@c CLOCK_REALTIME).
 *
 * @return 0 on success, -1 with @c errno = @c ETIMEDOUT on timeout,
 *         or -1 with errno set on another failure.
 *
 * @posix_func{sem_timedwait}
 */
int sem_timedwait(sem_t *ZRESTRICT semaphore, struct timespec *ZRESTRICT abstime);

/**
 * @brief Try to lock a semaphore without blocking.
 *
 * @param semaphore Semaphore to try.
 *
 * @return 0 on success, -1 with @c errno = @c EAGAIN if the count is zero,
 *         or -1 with errno set on another failure.
 *
 * @posix_func{sem_trywait}
 */
int sem_trywait(sem_t *semaphore);

/**
 * @brief Lock a semaphore, blocking until it becomes available.
 *
 * @param semaphore Semaphore to wait on.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sem_wait}
 */
int sem_wait(sem_t *semaphore);

/**
 * @brief Open or create a named semaphore.
 *
 * @param name   Semaphore name (implementation-defined path).
 * @param oflags Flags: @c O_CREAT, @c O_EXCL.
 * @param ...    If @c O_CREAT is set: mode_t mode, unsigned int value.
 *
 * @return Pointer to the semaphore on success, or @c SEM_FAILED on failure.
 *
 * @posix_func{sem_open}
 */
sem_t *sem_open(const char *name, int oflags, ...);

/**
 * @brief Remove a named semaphore.
 *
 * @param name Semaphore name.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sem_unlink}
 */
int sem_unlink(const char *name);

/**
 * @brief Close a named semaphore.
 *
 * @param sem Semaphore to close.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sem_close}
 */
int sem_close(sem_t *sem);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SEMAPHORE_H_ */
