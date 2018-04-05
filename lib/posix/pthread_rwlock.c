/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <errno.h>
#include <posix/time.h>
#include <posix/sys/types.h>

#define INITIALIZED 1
#define NOT_INITIALIZED 0

#define CONCURRENT_READER_LIMIT  (CONFIG_MAX_PTHREAD_COUNT + 1)

s64_t timespec_to_timeoutms(const struct timespec *abstime);
static u32_t read_lock_acquire(pthread_rwlock_t *rwlock, s32_t timeout);
static u32_t write_lock_acquire(pthread_rwlock_t *rwlock, s32_t timeout);

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

	return read_lock_acquire(rwlock, K_FOREVER);
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
	s32_t timeout;
	u32_t ret = 0;

	if (rwlock->status == NOT_INITIALIZED || abstime->tv_nsec < 0 ||
	    abstime->tv_nsec > NSEC_PER_SEC) {
		return EINVAL;
	}

	timeout = (s32_t) timespec_to_timeoutms(abstime);

	if (read_lock_acquire(rwlock, timeout) != 0) {
		ret = ETIMEDOUT;
	}

	return ret;
}

/**
 * @brief Lock a read-write lock object for reading immedately.
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

	return read_lock_acquire(rwlock, K_NO_WAIT);
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

	return write_lock_acquire(rwlock, K_FOREVER);
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
	s32_t timeout;
	u32_t ret = 0;

	if (rwlock->status == NOT_INITIALIZED || abstime->tv_nsec < 0 ||
	    abstime->tv_nsec > NSEC_PER_SEC) {
		return EINVAL;
	}

	timeout = (s32_t) timespec_to_timeoutms(abstime);

	if (write_lock_acquire(rwlock, timeout) != 0) {
		ret = ETIMEDOUT;
	}

	return ret;
}

/**
 * @brief Lock a read-write lock object for writing immedately.
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

	return write_lock_acquire(rwlock, K_NO_WAIT);
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
		if (k_sem_count_get(&rwlock->rd_sem) ==
				    (CONCURRENT_READER_LIMIT - 1)) {
			/* Last read lock, unlock writer */
			k_sem_give(&rwlock->reader_active);
		}

		k_sem_give(&rwlock->rd_sem);
	}
	return 0;
}


static u32_t read_lock_acquire(pthread_rwlock_t *rwlock, s32_t timeout)
{
	u32_t ret = 0;

	if (k_sem_take(&rwlock->wr_sem, timeout) == 0) {
		k_sem_take(&rwlock->reader_active, K_NO_WAIT);
		k_sem_take(&rwlock->rd_sem, K_NO_WAIT);
		k_sem_give(&rwlock->wr_sem);
	} else {
		ret = EBUSY;
	}

	return ret;
}

static u32_t write_lock_acquire(pthread_rwlock_t *rwlock, s32_t timeout)
{
	u32_t ret = 0;
	s64_t elapsed_time, st_time = k_uptime_get();

	/* waiting for release of write lock */
	if (k_sem_take(&rwlock->wr_sem, timeout) == 0) {
		if (timeout > K_NO_WAIT) {
			elapsed_time = k_uptime_get() - st_time;
			timeout = timeout <= elapsed_time ? K_NO_WAIT :
				  timeout - elapsed_time;
		}

		/* waiting for reader to complete operation */
		if (k_sem_take(&rwlock->reader_active, timeout) == 0) {
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


