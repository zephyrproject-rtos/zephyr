/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_internal.h"

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/sem.h>

LOG_MODULE_REGISTER(pthread_mutex, CONFIG_PTHREAD_MUTEX_LOG_LEVEL);

static SYS_SEM_DEFINE(lock, 1, 1);

int64_t timespec_to_timeoutms(const struct timespec *abstime);

#define MUTEX_MAX_REC_LOCK 32767

/*
 *  Default mutex attrs.
 */
static const struct pthread_mutexattr def_attr = {
	.type = PTHREAD_MUTEX_DEFAULT,
};

static struct k_mutex posix_mutex_pool[CONFIG_MAX_PTHREAD_MUTEX_COUNT];
static uint8_t posix_mutex_type[CONFIG_MAX_PTHREAD_MUTEX_COUNT];
SYS_BITARRAY_DEFINE_STATIC(posix_mutex_bitarray, CONFIG_MAX_PTHREAD_MUTEX_COUNT);

/*
 * We reserve the MSB to mark a pthread_mutex_t as initialized (from the
 * perspective of the application). With a linear space, this means that
 * the theoretical pthread_mutex_t range is [0,2147483647].
 */
BUILD_ASSERT(CONFIG_MAX_PTHREAD_MUTEX_COUNT < PTHREAD_OBJ_MASK_INIT,
	"CONFIG_MAX_PTHREAD_MUTEX_COUNT is too high");

static inline size_t posix_mutex_to_offset(struct k_mutex *m)
{
	return m - posix_mutex_pool;
}

static inline size_t to_posix_mutex_idx(pthread_mutex_t mut)
{
	return mark_pthread_obj_uninitialized(mut);
}

static struct k_mutex *get_posix_mutex(pthread_mutex_t mu)
{
	int actually_initialized;
	size_t bit = to_posix_mutex_idx(mu);

	/* if the provided mutex does not claim to be initialized, its invalid */
	if (!is_pthread_obj_initialized(mu)) {
		LOG_DBG("Mutex is uninitialized (%x)", mu);
		return NULL;
	}

	/* Mask off the MSB to get the actual bit index */
	if (sys_bitarray_test_bit(&posix_mutex_bitarray, bit, &actually_initialized) < 0) {
		LOG_DBG("Mutex is invalid (%x)", mu);
		return NULL;
	}

	if (actually_initialized == 0) {
		/* The mutex claims to be initialized but is actually not */
		LOG_DBG("Mutex claims to be initialized (%x)", mu);
		return NULL;
	}

	return &posix_mutex_pool[bit];
}

struct k_mutex *to_posix_mutex(pthread_mutex_t *mu)
{
	int err;
	size_t bit;
	struct k_mutex *m;

	if (*mu != PTHREAD_MUTEX_INITIALIZER) {
		return get_posix_mutex(*mu);
	}

	/* Try and automatically associate a posix_mutex */
	if (sys_bitarray_alloc(&posix_mutex_bitarray, 1, &bit) < 0) {
		LOG_DBG("Unable to allocate pthread_mutex_t");
		return NULL;
	}

	/* Record the associated posix_mutex in mu and mark as initialized */
	*mu = mark_pthread_obj_initialized(bit);

	/* Initialize the posix_mutex */
	m = &posix_mutex_pool[bit];

	err = k_mutex_init(m);
	__ASSERT_NO_MSG(err == 0);

	return m;
}

static int acquire_mutex(pthread_mutex_t *mu, k_timeout_t timeout)
{
	int type = -1;
	size_t bit = -1;
	int ret = EINVAL;
	size_t lock_count = -1;
	struct k_mutex *m = NULL;
	struct k_thread *owner = NULL;

	SYS_SEM_LOCK(&lock) {
		m = to_posix_mutex(mu);
		if (m == NULL) {
			ret = EINVAL;
			SYS_SEM_LOCK_BREAK;
		}

		LOG_DBG("Locking mutex %p with timeout %llx", m, timeout.ticks);

		ret = 0;
		bit = posix_mutex_to_offset(m);
		type = posix_mutex_type[bit];
		owner = m->owner;
		lock_count = m->lock_count;
	}

	if (ret != 0) {
		goto handle_error;
	}

	if (owner == k_current_get()) {
		switch (type) {
		case PTHREAD_MUTEX_NORMAL:
			if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
				LOG_DBG("Timeout locking mutex %p", m);
				ret = EBUSY;
				break;
			}
			/* On most POSIX systems, this usually results in an infinite loop */
			LOG_DBG("Attempt to relock non-recursive mutex %p", m);
			do {
				(void)k_sleep(K_FOREVER);
			} while (true);
			CODE_UNREACHABLE;
			break;
		case PTHREAD_MUTEX_RECURSIVE:
			if (lock_count >= MUTEX_MAX_REC_LOCK) {
				LOG_DBG("Mutex %p locked recursively too many times", m);
				ret = EAGAIN;
			}
			break;
		case PTHREAD_MUTEX_ERRORCHECK:
			LOG_DBG("Attempt to recursively lock non-recursive mutex %p", m);
			ret = EDEADLK;
			break;
		default:
			__ASSERT(false, "invalid pthread type %d", type);
			ret = EINVAL;
			break;
		}
	}

	if (ret == 0) {
		ret = k_mutex_lock(m, timeout);
		if (ret == -EAGAIN) {
			LOG_DBG("Timeout locking mutex %p", m);
			/*
			 * special quirk - k_mutex_lock() returns EAGAIN if a timeout occurs, but
			 * for pthreads, that means something different
			 */
			ret = ETIMEDOUT;
		}
	}

handle_error:
	if (ret < 0) {
		LOG_DBG("k_mutex_unlock() failed: %d", ret);
		ret = -ret;
	}

	if (ret == 0) {
		LOG_DBG("Locked mutex %p", m);
	}

	return ret;
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
	size_t bit;
	struct k_mutex *m;
	const struct pthread_mutexattr *attr = (const struct pthread_mutexattr *)_attr;

	*mu = PTHREAD_MUTEX_INITIALIZER;

	m = to_posix_mutex(mu);
	if (m == NULL) {
		return ENOMEM;
	}

	bit = posix_mutex_to_offset(m);
	if (attr == NULL) {
		posix_mutex_type[bit] = def_attr.type;
	} else {
		posix_mutex_type[bit] = attr->type;
	}

	LOG_DBG("Initialized mutex %p", m);

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
	int ret;
	struct k_mutex *m;

	m = get_posix_mutex(*mu);
	if (m == NULL) {
		return EINVAL;
	}

	ret = k_mutex_unlock(m);
	if (ret < 0) {
		LOG_DBG("k_mutex_unlock() failed: %d", ret);
		return -ret;
	}

	__ASSERT_NO_MSG(ret == 0);
	LOG_DBG("Unlocked mutex %p", m);

	return 0;
}

/**
 * @brief Destroy POSIX mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutex_destroy(pthread_mutex_t *mu)
{
	int err;
	size_t bit;
	struct k_mutex *m;

	m = get_posix_mutex(*mu);
	if (m == NULL) {
		return EINVAL;
	}

	bit = to_posix_mutex_idx(*mu);
	err = sys_bitarray_free(&posix_mutex_bitarray, 1, bit);
	__ASSERT_NO_MSG(err == 0);

	LOG_DBG("Destroyed mutex %p", m);

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
	if ((attr == NULL) || (protocol == NULL)) {
		return EINVAL;
	}

	*protocol = PTHREAD_PRIO_NONE;
	return 0;
}

/**
 * @brief Set protocol attribute for mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol)
{
	if (attr == NULL) {
		return EINVAL;
	}

	switch (protocol) {
	case PTHREAD_PRIO_NONE:
		return 0;
	case PTHREAD_PRIO_INHERIT:
		return ENOTSUP;
	case PTHREAD_PRIO_PROTECT:
		return ENOTSUP;
	default:
		return EINVAL;
	}
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	struct pthread_mutexattr *const a = (struct pthread_mutexattr *)attr;

	if (a == NULL) {
		return EINVAL;
	}

	a->type = PTHREAD_MUTEX_DEFAULT;
	a->initialized = true;

	return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
	struct pthread_mutexattr *const a = (struct pthread_mutexattr *)attr;

	if (a == NULL || !a->initialized) {
		return EINVAL;
	}

	*a = (struct pthread_mutexattr){0};

	return 0;
}

/**
 * @brief Read type attribute for mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
	const struct pthread_mutexattr *a = (const struct pthread_mutexattr *)attr;

	if (a == NULL || type == NULL || !a->initialized) {
		return EINVAL;
	}

	*type = a->type;

	return 0;
}

/**
 * @brief Set type attribute for mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	struct pthread_mutexattr *const a = (struct pthread_mutexattr *)attr;

	if (a == NULL || !a->initialized) {
		return EINVAL;
	}

	switch (type) {
	case PTHREAD_MUTEX_NORMAL:
	case PTHREAD_MUTEX_RECURSIVE:
	case PTHREAD_MUTEX_ERRORCHECK:
		a->type = type;
		return 0;
	default:
		return EINVAL;
	}
}

#ifdef CONFIG_POSIX_THREAD_PRIO_PROTECT
int pthread_mutex_getprioceiling(const pthread_mutex_t *mutex, int *prioceiling)
{
	ARG_UNUSED(mutex);
	ARG_UNUSED(prioceiling);

	return ENOSYS;
}

int pthread_mutex_setprioceiling(pthread_mutex_t *mutex, int prioceiling, int *old_ceiling)
{
	ARG_UNUSED(mutex);
	ARG_UNUSED(prioceiling);
	ARG_UNUSED(old_ceiling);

	return ENOSYS;
}

int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *attr, int *prioceiling)
{
	ARG_UNUSED(attr);
	ARG_UNUSED(prioceiling);

	return ENOSYS;
}

int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, int prioceiling)
{
	ARG_UNUSED(attr);
	ARG_UNUSED(prioceiling);

	return ENOSYS;
}

#endif /* CONFIG_POSIX_THREAD_PRIO_PROTECT */

static int pthread_mutex_pool_init(void)
{
	int err;
	size_t i;

	for (i = 0; i < CONFIG_MAX_PTHREAD_MUTEX_COUNT; ++i) {
		err = k_mutex_init(&posix_mutex_pool[i]);
		__ASSERT_NO_MSG(err == 0);
	}

	return 0;
}
SYS_INIT(pthread_mutex_pool_init, PRE_KERNEL_1, 0);
