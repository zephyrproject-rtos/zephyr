/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/wait_q.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/bitarray.h>

#include "posix_internal.h"

struct k_spinlock z_pthread_spinlock;

int64_t timespec_to_timeoutms(const struct timespec *abstime);

#define MUTEX_MAX_REC_LOCK 32767

/*
 *  Default mutex attrs.
 */
static const struct pthread_mutexattr def_attr = {
	.type = PTHREAD_MUTEX_DEFAULT,
};

static struct posix_mutex posix_mutex_pool[CONFIG_MAX_PTHREAD_MUTEX_COUNT];
SYS_BITARRAY_DEFINE_STATIC(posix_mutex_bitarray, CONFIG_MAX_PTHREAD_MUTEX_COUNT);

/*
 * We reserve the MSB to mark a pthread_mutex_t as initialized (from the
 * perspective of the application). With a linear space, this means that
 * the theoretical pthread_mutex_t range is [0,2147483647].
 */
BUILD_ASSERT(CONFIG_MAX_PTHREAD_MUTEX_COUNT < PTHREAD_OBJ_MASK_INIT,
	"CONFIG_MAX_PTHREAD_MUTEX_COUNT is too high");

static inline size_t posix_mutex_to_offset(struct posix_mutex *m)
{
	return m - posix_mutex_pool;
}

static inline size_t to_posix_mutex_idx(pthread_mutex_t mut)
{
	return mark_pthread_obj_uninitialized(mut);
}

struct posix_mutex *get_posix_mutex(pthread_mutex_t mu)
{
	int actually_initialized;
	size_t bit = to_posix_mutex_idx(mu);

	/* if the provided mutex does not claim to be initialized, its invalid */
	if (!is_pthread_obj_initialized(mu)) {
		return NULL;
	}

	/* Mask off the MSB to get the actual bit index */
	if (sys_bitarray_test_bit(&posix_mutex_bitarray, bit, &actually_initialized) < 0) {
		return NULL;
	}

	if (actually_initialized == 0) {
		/* The mutex claims to be initialized but is actually not */
		return NULL;
	}

	return &posix_mutex_pool[bit];
}

struct posix_mutex *to_posix_mutex(pthread_mutex_t *mu)
{
	size_t bit;
	struct posix_mutex *m;

	if (*mu != PTHREAD_MUTEX_INITIALIZER) {
		return get_posix_mutex(*mu);
	}

	/* Try and automatically associate a posix_mutex */
	if (sys_bitarray_alloc(&posix_mutex_bitarray, 1, &bit) < 0) {
		/* No mutexes left to allocate */
		return NULL;
	}

	/* Record the associated posix_mutex in mu and mark as initialized */
	*mu = mark_pthread_obj_initialized(bit);

	/* Initialize the posix_mutex */
	m = &posix_mutex_pool[bit];

	m->owner = NULL;
	m->lock_count = 0U;

	z_waitq_init(&m->wait_q);

	return m;
}

static int acquire_mutex(pthread_mutex_t *mu, k_timeout_t timeout)
{
	int rc = 0;
	k_spinlock_key_t key;
	struct posix_mutex *m;

	key = k_spin_lock(&z_pthread_spinlock);

	m = to_posix_mutex(mu);
	if (m == NULL) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return EINVAL;
	}

	if (m->lock_count == 0U && m->owner == NULL) {
		m->lock_count++;
		m->owner = k_current_get();

		k_spin_unlock(&z_pthread_spinlock, key);
		return 0;
	} else if (m->owner == k_current_get()) {
		if (m->type == PTHREAD_MUTEX_RECURSIVE &&
		    m->lock_count < MUTEX_MAX_REC_LOCK) {
			m->lock_count++;
			rc = 0;
		} else if (m->type == PTHREAD_MUTEX_ERRORCHECK) {
			rc = EDEADLK;
		} else {
			rc = EINVAL;
		}

		k_spin_unlock(&z_pthread_spinlock, key);
		return rc;
	}

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return EINVAL;
	}

	rc = z_pend_curr(&z_pthread_spinlock, key, &m->wait_q, timeout);
	if (rc != 0) {
		rc = ETIMEDOUT;
	}

	return rc;
}

/**
 * @brief Lock POSIX mutex with non-blocking call.
 *
 * See IEEE 1003.1
 */
int pthread_mutex_trylock(pthread_mutex_t *m)
{
	return acquire_mutex(m, K_NO_WAIT);
}

/**
 * @brief Lock POSIX mutex with timeout.
 *
 *
 * See IEEE 1003.1
 */
int pthread_mutex_timedlock(pthread_mutex_t *m,
			    const struct timespec *abstime)
{
	int32_t timeout = (int32_t)timespec_to_timeoutms(abstime);
	return acquire_mutex(m, K_MSEC(timeout));
}

/**
 * @brief Initialize POSIX mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutex_init(pthread_mutex_t *mu, const pthread_mutexattr_t *_attr)
{
	k_spinlock_key_t key;
	struct posix_mutex *m;
	const struct pthread_mutexattr *attr = (const struct pthread_mutexattr *)_attr;

	*mu = PTHREAD_MUTEX_INITIALIZER;
	key = k_spin_lock(&z_pthread_spinlock);

	m = to_posix_mutex(mu);
	if (m == NULL) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return ENOMEM;
	}

	m->type = (attr == NULL) ? def_attr.type : attr->type;

	k_spin_unlock(&z_pthread_spinlock, key);

	return 0;
}


/**
 * @brief Lock POSIX mutex with blocking call.
 *
 * See IEEE 1003.1
 */
int pthread_mutex_lock(pthread_mutex_t *m)
{
	return acquire_mutex(m, K_FOREVER);
}

/**
 * @brief Unlock POSIX mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutex_unlock(pthread_mutex_t *mu)
{
	k_tid_t thread;
	k_spinlock_key_t key;
	struct posix_mutex *m;
	pthread_mutex_t mut = *mu;

	key = k_spin_lock(&z_pthread_spinlock);

	m = get_posix_mutex(mut);
	if (m == NULL) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return EINVAL;
	}

	if (m->owner != k_current_get()) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return EPERM;
	}

	if (m->lock_count == 0U) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return EINVAL;
	}

	m->lock_count--;

	if (m->lock_count == 0U) {
		thread = z_unpend_first_thread(&m->wait_q);
		if (thread) {
			m->owner = thread;
			m->lock_count++;
			arch_thread_return_value_set(thread, 0);
			z_ready_thread(thread);
			z_reschedule(&z_pthread_spinlock, key);
			return 0;
		}
		m->owner = NULL;

	}
	k_spin_unlock(&z_pthread_spinlock, key);
	return 0;
}

/**
 * @brief Destroy POSIX mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutex_destroy(pthread_mutex_t *mu)
{
	__unused int rc;
	k_spinlock_key_t key;
	struct posix_mutex *m;
	pthread_mutex_t mut = *mu;
	size_t bit = to_posix_mutex_idx(mut);

	key = k_spin_lock(&z_pthread_spinlock);
	m = get_posix_mutex(mut);
	if (m == NULL) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return EINVAL;
	}

	rc = sys_bitarray_free(&posix_mutex_bitarray, 1, bit);
	__ASSERT(rc == 0, "failed to free bit %zu", bit);

	k_spin_unlock(&z_pthread_spinlock, key);

	return 0;
}

/**
 * @brief Read protocol attribute for mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr,
				  int *protocol)
{
	*protocol = PTHREAD_PRIO_NONE;
	return 0;
}

/**
 * @brief Read type attribute for mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutexattr_gettype(const pthread_mutexattr_t *_attr, int *type)
{
	const struct pthread_mutexattr *attr = (const struct pthread_mutexattr *)_attr;
	*type = attr->type;
	return 0;
}

/**
 * @brief Set type attribute for mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutexattr_settype(pthread_mutexattr_t *_attr, int type)
{
	struct pthread_mutexattr *attr = (struct pthread_mutexattr *)_attr;
	int retc = EINVAL;

	if ((type == PTHREAD_MUTEX_NORMAL) ||
	    (type == PTHREAD_MUTEX_RECURSIVE) ||
	    (type == PTHREAD_MUTEX_ERRORCHECK)) {
		attr->type = type;
		retc = 0;
	}

	return retc;
}
