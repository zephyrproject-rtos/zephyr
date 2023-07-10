/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/posix/time.h>
#include <zephyr/posix/posix_types.h>

#define INITIALIZED 1
#define NOT_INITIALIZED 0

#define CONCURRENT_READER_LIMIT  (CONFIG_MAX_PTHREAD_COUNT + 1)

int64_t timespec_to_timeoutms(const struct timespec *abstime);
static uint32_t read_lock_acquire(pthread_rwlock_t *rwlock, int32_t timeout);
static uint32_t write_lock_acquire(pthread_rwlock_t *rwlock, int32_t timeout);

/**
 * @brief Initialize read-write lock object.
 *
 * See IEEE 1003.1
 */
int pthread_rwlock_init(pthread_rwlock_t *rwlock,
			const pthread_rwlockattr_t *attr)
{
	k_sem_init(&rwlock->rd_sem, CONCURRENT_READER_LIMIT,
		   CONCURRENT_READER_LIMIT);
	k_sem_init(&rwlock->wr_sem, 1, 1);
	k_sem_init(&rwlock->reader_active, 1, 1);
	rwlock->wr_owner = NULL;
	rwlock->status = INITIALIZED;
	return 0;
}

/**
 * @brief Destroy read-write lock object.
 *
 * See IEEE 1003.1
 */
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
	if (rwlock->status == NOT_INITIALIZED) {
		return EINVAL;
	}

	if (rwlock->wr_owner != NULL) {
		return EBUSY;
	}

	if (rwlock->status == INITIALIZED) {
		rwlock->status = NOT_INITIALIZED;
		return 0;
	}

	return EINVAL;
}

/**
 * @brief Lock a read-write lock object for reading.
 *
 * API behaviour is unpredictable if number of concurrent reader
 * lock held is greater than CONCURRENT_READER_LIMIT.
 *
 * See IEEE 1003.1
 */
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
	if (rwlock->status == NOT_INITIALIZED) {
		return EINVAL;
	}

	return read_lock_acquire(rwlock, SYS_FOREVER_MS);
}

/**
 * @brief Lock a read-write lock object for reading within specific time.
 *
 * API behaviour is unpredictable if number of concurrent reader
 * lock held is greater than CONCURRENT_READER_LIMIT.
 *
 * See IEEE 1003.1
 */
int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock,
			       const struct timespec *abstime)
{
	int32_t timeout;
	uint32_t ret = 0U;

	if (rwlock->status == NOT_INITIALIZED || abstime->tv_nsec < 0 ||
	    abstime->tv_nsec > NSEC_PER_SEC) {
		return EINVAL;
	}

	timeout = (int32_t) timespec_to_timeoutms(abstime);

	if (read_lock_acquire(rwlock, timeout) != 0U) {
		ret = ETIMEDOUT;
	}

	return ret;
}

/**
 * @brief Lock a read-write lock object for reading immediately.
 *
 * API behaviour is unpredictable if number of concurrent reader
 * lock held is greater than CONCURRENT_READER_LIMIT.
 *
 * See IEEE 1003.1
 */
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
	if (rwlock->status == NOT_INITIALIZED) {
		return EINVAL;
	}

	return read_lock_acquire(rwlock, 0);
}

/**
 * @brief Lock a read-write lock object for writing.
 *
 * Write lock does not have priority over reader lock,
 * threads get lock based on priority.
 *
 * See IEEE 1003.1
 */
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
	if (rwlock->status == NOT_INITIALIZED) {
		return EINVAL;
	}

	return write_lock_acquire(rwlock, SYS_FOREVER_MS);
}

/**
 * @brief Lock a read-write lock object for writing within specific time.
 *
 * Write lock does not have priority over reader lock,
 * threads get lock based on priority.
 *
 * See IEEE 1003.1
 */
int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock,
			       const struct timespec *abstime)
{
	int32_t timeout;
	uint32_t ret = 0U;

	if (rwlock->status == NOT_INITIALIZED || abstime->tv_nsec < 0 ||
	    abstime->tv_nsec > NSEC_PER_SEC) {
		return EINVAL;
	}

	timeout = (int32_t) timespec_to_timeoutms(abstime);

	if (write_lock_acquire(rwlock, timeout) != 0U) {
		ret = ETIMEDOUT;
	}

	return ret;
}

/**
 * @brief Lock a read-write lock object for writing immediately.
 *
 * Write lock does not have priority over reader lock,
 * threads get lock based on priority.
 *
 * See IEEE 1003.1
 */
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
	if (rwlock->status == NOT_INITIALIZED) {
		return EINVAL;
	}

	return write_lock_acquire(rwlock, 0);
}

/**
 *
 * @brief Unlock a read-write lock object.
 *
 * See IEEE 1003.1
 */
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
	if (rwlock->status == NOT_INITIALIZED) {
		return EINVAL;
	}

	if (k_current_get() == rwlock->wr_owner) {
		/* Write unlock */
		rwlock->wr_owner = NULL;
		k_sem_give(&rwlock->reader_active);
		k_sem_give(&rwlock->wr_sem);
	} else {
		/* Read unlock */
		k_sem_give(&rwlock->rd_sem);

		if (k_sem_count_get(&rwlock->rd_sem) ==
				    CONCURRENT_READER_LIMIT) {
			/* Last read lock, unlock writer */
			k_sem_give(&rwlock->reader_active);
		}
	}
	return 0;
}


static uint32_t read_lock_acquire(pthread_rwlock_t *rwlock, int32_t timeout)
{
	uint32_t ret = 0U;

	if (k_sem_take(&rwlock->wr_sem, SYS_TIMEOUT_MS(timeout)) == 0) {
		k_sem_take(&rwlock->reader_active, K_NO_WAIT);
		k_sem_take(&rwlock->rd_sem, K_NO_WAIT);
		k_sem_give(&rwlock->wr_sem);
	} else {
		ret = EBUSY;
	}

	return ret;
}

static uint32_t write_lock_acquire(pthread_rwlock_t *rwlock, int32_t timeout)
{
	uint32_t ret = 0U;
	int64_t elapsed_time, st_time = k_uptime_get();
	k_timeout_t k_timeout;

	k_timeout = SYS_TIMEOUT_MS(timeout);

	/* waiting for release of write lock */
	if (k_sem_take(&rwlock->wr_sem, k_timeout) == 0) {
		/* update remaining timeout time for 2nd sem */
		if (timeout != SYS_FOREVER_MS) {
			elapsed_time = k_uptime_get() - st_time;
			timeout = timeout <= elapsed_time ? 0 :
				  timeout - elapsed_time;
		}

		k_timeout = SYS_TIMEOUT_MS(timeout);

		/* waiting for reader to complete operation */
		if (k_sem_take(&rwlock->reader_active, k_timeout) == 0) {
			rwlock->wr_owner = k_current_get();
		} else {
			k_sem_give(&rwlock->wr_sem);
			ret = EBUSY;
		}

	} else {
		ret = EBUSY;
	}
	return ret;
}
