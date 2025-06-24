/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_clock.h"
#include "posix_internal.h"

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/sem.h>

#define CONCURRENT_READER_LIMIT  (CONFIG_POSIX_THREAD_THREADS_MAX + 1)

struct posix_rwlock {
	struct sys_sem rd_sem;
	struct sys_sem wr_sem;
	struct sys_sem reader_active; /* blocks WR till reader has acquired lock */
	k_tid_t wr_owner;
};

struct posix_rwlockattr {
	bool initialized: 1;
	bool pshared: 1;
};

static uint32_t read_lock_acquire(struct posix_rwlock *rwl, uint32_t timeout);
static uint32_t write_lock_acquire(struct posix_rwlock *rwl, uint32_t timeout);

LOG_MODULE_REGISTER(pthread_rwlock, CONFIG_PTHREAD_RWLOCK_LOG_LEVEL);

static SYS_SEM_DEFINE(posix_rwlock_lock, 1, 1);

static struct posix_rwlock posix_rwlock_pool[CONFIG_MAX_PTHREAD_RWLOCK_COUNT];
SYS_BITARRAY_DEFINE_STATIC(posix_rwlock_bitarray, CONFIG_MAX_PTHREAD_RWLOCK_COUNT);

/*
 * We reserve the MSB to mark a pthread_rwlock_t as initialized (from the
 * perspective of the application). With a linear space, this means that
 * the theoretical pthread_rwlock_t range is [0,2147483647].
 */
BUILD_ASSERT(CONFIG_MAX_PTHREAD_RWLOCK_COUNT < PTHREAD_OBJ_MASK_INIT,
	     "CONFIG_MAX_PTHREAD_RWLOCK_COUNT is too high");

static inline size_t posix_rwlock_to_offset(struct posix_rwlock *rwl)
{
	return rwl - posix_rwlock_pool;
}

static inline size_t to_posix_rwlock_idx(pthread_rwlock_t rwlock)
{
	return mark_pthread_obj_uninitialized(rwlock);
}

static struct posix_rwlock *get_posix_rwlock(pthread_rwlock_t rwlock)
{
	int actually_initialized;
	size_t bit = to_posix_rwlock_idx(rwlock);

	/* if the provided rwlock does not claim to be initialized, its invalid */
	if (!is_pthread_obj_initialized(rwlock)) {
		LOG_DBG("RWlock is uninitialized (%x)", rwlock);
		return NULL;
	}

	/* Mask off the MSB to get the actual bit index */
	if (sys_bitarray_test_bit(&posix_rwlock_bitarray, bit, &actually_initialized) < 0) {
		LOG_DBG("RWlock is invalid (%x)", rwlock);
		return NULL;
	}

	if (actually_initialized == 0) {
		/* The rwlock claims to be initialized but is actually not */
		LOG_DBG("RWlock claims to be initialized (%x)", rwlock);
		return NULL;
	}

	return &posix_rwlock_pool[bit];
}

struct posix_rwlock *to_posix_rwlock(pthread_rwlock_t *rwlock)
{
	size_t bit;
	struct posix_rwlock *rwl;

	if (*rwlock != PTHREAD_RWLOCK_INITIALIZER) {
		return get_posix_rwlock(*rwlock);
	}

	/* Try and automatically associate a posix_rwlock */
	if (sys_bitarray_alloc(&posix_rwlock_bitarray, 1, &bit) < 0) {
		LOG_DBG("Unable to allocate pthread_rwlock_t");
		return NULL;
	}

	/* Record the associated posix_rwlock in rwl and mark as initialized */
	*rwlock = mark_pthread_obj_initialized(bit);

	/* Initialize the posix_rwlock */
	rwl = &posix_rwlock_pool[bit];

	return rwl;
}

/**
 * @brief Initialize read-write lock object.
 *
 * See IEEE 1003.1
 */
int pthread_rwlock_init(pthread_rwlock_t *rwlock,
			const pthread_rwlockattr_t *attr)
{
	struct posix_rwlock *rwl;

	ARG_UNUSED(attr);
	*rwlock = PTHREAD_RWLOCK_INITIALIZER;

	rwl = to_posix_rwlock(rwlock);
	if (rwl == NULL) {
		return ENOMEM;
	}

	sys_sem_init(&rwl->rd_sem, CONCURRENT_READER_LIMIT, CONCURRENT_READER_LIMIT);
	sys_sem_init(&rwl->wr_sem, 1, 1);
	sys_sem_init(&rwl->reader_active, 1, 1);
	rwl->wr_owner = NULL;

	LOG_DBG("Initialized rwlock %p", rwl);

	return 0;
}

/**
 * @brief Destroy read-write lock object.
 *
 * See IEEE 1003.1
 */
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
	int err;
	size_t bit;
	int ret = EINVAL;
	struct posix_rwlock *rwl;

	SYS_SEM_LOCK(&posix_rwlock_lock) {
		rwl = get_posix_rwlock(*rwlock);
		if (rwl == NULL) {
			ret = EINVAL;
			SYS_SEM_LOCK_BREAK;
		}

		if (rwl->wr_owner != NULL) {
			ret = EBUSY;
			SYS_SEM_LOCK_BREAK;
		}

		ret = 0;
		bit = posix_rwlock_to_offset(rwl);
		err = sys_bitarray_free(&posix_rwlock_bitarray, 1, bit);
		__ASSERT_NO_MSG(err == 0);
	}

	return ret;
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
	struct posix_rwlock *rwl;

	rwl = get_posix_rwlock(*rwlock);
	if (rwl == NULL) {
		return EINVAL;
	}

	return read_lock_acquire(rwl, SYS_FOREVER_MS);
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
	uint32_t ret = 0U;
	struct posix_rwlock *rwl;

	if ((abstime == NULL) || !timespec_is_valid(abstime)) {
		LOG_DBG("%s is invalid", "abstime");
		return EINVAL;
	}

	rwl = get_posix_rwlock(*rwlock);
	if (rwl == NULL) {
		return EINVAL;
	}

	if (read_lock_acquire(rwl, timespec_to_timeoutms(abstime)) != 0U) {
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
	struct posix_rwlock *rwl;

	rwl = get_posix_rwlock(*rwlock);
	if (rwl == NULL) {
		return EINVAL;
	}

	return read_lock_acquire(rwl, 0);
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
	struct posix_rwlock *rwl;

	rwl = get_posix_rwlock(*rwlock);
	if (rwl == NULL) {
		return EINVAL;
	}

	return write_lock_acquire(rwl, SYS_FOREVER_MS);
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
	uint32_t ret = 0U;
	struct posix_rwlock *rwl;

	if ((abstime == NULL) || !timespec_is_valid(abstime)) {
		LOG_DBG("%s is invalid", "abstime");
		return EINVAL;
	}

	rwl = get_posix_rwlock(*rwlock);
	if (rwl == NULL) {
		return EINVAL;
	}

	if (write_lock_acquire(rwl, timespec_to_timeoutms(abstime)) != 0U) {
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
	struct posix_rwlock *rwl;

	rwl = get_posix_rwlock(*rwlock);
	if (rwl == NULL) {
		return EINVAL;
	}

	return write_lock_acquire(rwl, 0);
}

/**
 *
 * @brief Unlock a read-write lock object.
 *
 * See IEEE 1003.1
 */
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
	struct posix_rwlock *rwl;

	rwl = get_posix_rwlock(*rwlock);
	if (rwl == NULL) {
		return EINVAL;
	}

	if (k_current_get() == rwl->wr_owner) {
		/* Write unlock */
		rwl->wr_owner = NULL;
		(void)sys_sem_give(&rwl->reader_active);
		(void)sys_sem_give(&rwl->wr_sem);
	} else {
		/* Read unlock */
		(void)sys_sem_give(&rwl->rd_sem);

		if (sys_sem_count_get(&rwl->rd_sem) == CONCURRENT_READER_LIMIT) {
			/* Last read lock, unlock writer */
			(void)sys_sem_give(&rwl->reader_active);
		}
	}
	return 0;
}

static uint32_t read_lock_acquire(struct posix_rwlock *rwl, uint32_t timeout)
{
	uint32_t ret = 0U;

	if (sys_sem_take(&rwl->wr_sem, SYS_TIMEOUT_MS(timeout)) == 0) {
		(void)sys_sem_take(&rwl->reader_active, K_NO_WAIT);
		(void)sys_sem_take(&rwl->rd_sem, K_NO_WAIT);
		(void)sys_sem_give(&rwl->wr_sem);
	} else {
		ret = EBUSY;
	}

	return ret;
}

static uint32_t write_lock_acquire(struct posix_rwlock *rwl, uint32_t timeout)
{
	uint32_t ret = 0U;
	int64_t elapsed_time, st_time = k_uptime_get();
	k_timeout_t k_timeout;

	k_timeout = SYS_TIMEOUT_MS(timeout);

	/* waiting for release of write lock */
	if (sys_sem_take(&rwl->wr_sem, k_timeout) == 0) {
		/* update remaining timeout time for 2nd sem */
		if (timeout != SYS_FOREVER_MS) {
			elapsed_time = k_uptime_get() - st_time;
			timeout = timeout <= elapsed_time ? 0 :
				  timeout - elapsed_time;
		}

		k_timeout = SYS_TIMEOUT_MS(timeout);

		/* waiting for reader to complete operation */
		if (sys_sem_take(&rwl->reader_active, k_timeout) == 0) {
			rwl->wr_owner = k_current_get();
		} else {
			(void)sys_sem_give(&rwl->wr_sem);
			ret = EBUSY;
		}

	} else {
		ret = EBUSY;
	}
	return ret;
}

int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *ZRESTRICT attr,
				  int *ZRESTRICT pshared)
{
	struct posix_rwlockattr *const a = (struct posix_rwlockattr *)attr;

	if (a == NULL || !a->initialized) {
		return EINVAL;
	}

	*pshared = a->pshared;

	return 0;
}

int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared)
{
	struct posix_rwlockattr *const a = (struct posix_rwlockattr *)attr;

	if (a == NULL || !a->initialized) {
		return EINVAL;
	}

	if (!(pshared == PTHREAD_PROCESS_PRIVATE || pshared == PTHREAD_PROCESS_SHARED)) {
		return EINVAL;
	}

	a->pshared = pshared;

	return 0;
}

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr)
{
	struct posix_rwlockattr *const a = (struct posix_rwlockattr *)attr;

	if (a == NULL) {
		return EINVAL;
	}

	*a = (struct posix_rwlockattr){
		.initialized = true,
		.pshared = PTHREAD_PROCESS_PRIVATE,
	};

	return 0;
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
{
	struct posix_rwlockattr *const a = (struct posix_rwlockattr *)attr;

	if (a == NULL || !a->initialized) {
		return EINVAL;
	}

	*a = (struct posix_rwlockattr){0};

	return 0;
}
