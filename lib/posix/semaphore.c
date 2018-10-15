/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <posix/pthread.h>

/**
 * @brief Destroy semaphore.
 *
 * see IEEE 1003.1
 */
int sem_destroy(sem_t *semaphore)
{
	if (semaphore == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (k_sem_count_get(semaphore)) {
		errno = EBUSY;
		return -1;
	}

	k_sem_reset(semaphore);
	return 0;
}

/**
 * @brief Get value of semaphore.
 *
 * See IEEE 1003.1
 */
int sem_getvalue(sem_t *semaphore, int *value)
{
	if (semaphore == NULL) {
		errno = EINVAL;
		return -1;
	}

	*value = (int) k_sem_count_get(semaphore);

	return 0;
}
/**
 * @brief Initialize semaphore.
 *
 * See IEEE 1003.1
 */
int sem_init(sem_t *semaphore, int pshared, unsigned int value)
{
	if (value > CONFIG_SEM_VALUE_MAX) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * Zephyr has no concept of process, so only thread shared
	 * semaphore makes sense in here.
	 */
	__ASSERT(pshared == 0, "pshared should be 0");

	k_sem_init(semaphore, value, CONFIG_SEM_VALUE_MAX);

	return 0;
}

/**
 * @brief Unlock a semaphore.
 *
 * See IEEE 1003.1
 */
int sem_post(sem_t *semaphore)
{
	if (semaphore == NULL) {
		errno = EINVAL;
		return -1;
	}

	k_sem_give(semaphore);
	return 0;
}

/**
 * @brief Try time limited locking a semaphore.
 *
 * See IEEE 1003.1
 */
int sem_timedwait(sem_t *semaphore, struct timespec *abstime)
{
	s32_t timeout;
	s64_t current_ms, abstime_ms;

	__ASSERT(abstime, "abstime pointer NULL");

	if ((abstime->tv_sec < 0) || (abstime->tv_nsec >= NSEC_PER_SEC)) {
		errno = EINVAL;
		return -1;
	}

	current_ms = (s64_t)k_uptime_get();
	abstime_ms = (s64_t)_ts_to_ms(abstime);

	if (abstime_ms <= current_ms) {
		timeout = K_NO_WAIT;
	} else {
		timeout = (s32_t)(abstime_ms - current_ms);
	}

	if (k_sem_take(semaphore, timeout)) {
		errno = ETIMEDOUT;
		return -1;
	}

	return 0;
}

/**
 * @brief Lock a semaphore if not taken.
 *
 * See IEEE 1003.1
 */
int sem_trywait(sem_t *semaphore)
{
	if (k_sem_take(semaphore, K_NO_WAIT) == -EBUSY) {
		errno = EAGAIN;
		return -1;
	} else {
		return 0;
	}
}

/**
 * @brief Lock a semaphore.
 *
 * See IEEE 1003.1
 */
int sem_wait(sem_t *semaphore)
{
	k_sem_take(semaphore, K_FOREVER);
	return 0;
}
