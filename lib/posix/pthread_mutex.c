/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <wait_q.h>
#include <posix/pthread.h>

int64_t timespec_to_timeoutms(const struct timespec *abstime);

#define MUTEX_MAX_REC_LOCK 32767

/*
 *  Default mutex attrs.
 */
static const pthread_mutexattr_t def_attr = {
	.type = PTHREAD_MUTEX_DEFAULT,
};

static int acquire_mutex(pthread_mutex_t *m, k_timeout_t timeout)
{
	int rc = 0, key = irq_lock();

	if (m->lock_count == 0U && m->owner == NULL) {
		m->lock_count++;
		m->owner = pthread_self();

		irq_unlock(key);
		return 0;
	} else if (m->owner == pthread_self()) {
		if (m->type == PTHREAD_MUTEX_RECURSIVE &&
		    m->lock_count < MUTEX_MAX_REC_LOCK) {
			m->lock_count++;
			rc = 0;
		} else if (m->type == PTHREAD_MUTEX_ERRORCHECK) {
			rc = EDEADLK;
		} else {
			rc = EINVAL;
		}

		irq_unlock(key);
		return rc;
	}

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		irq_unlock(key);
		return EINVAL;
	}

	rc = z_pend_curr_irqlock(key, &m->wait_q, timeout);
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
 * @brief Intialize POSIX mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutex_init(pthread_mutex_t *m,
				     const pthread_mutexattr_t *attr)
{
	const pthread_mutexattr_t *mattr;

	m->owner = NULL;
	m->lock_count = 0U;

	mattr = (attr == NULL) ? &def_attr : attr;

	m->type = mattr->type;

	z_waitq_init(&m->wait_q);

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
int pthread_mutex_unlock(pthread_mutex_t *m)
{
	unsigned int key = irq_lock();

	k_tid_t thread;

	if (m->owner != pthread_self()) {
		irq_unlock(key);
		return EPERM;
	}

	if (m->lock_count == 0U) {
		irq_unlock(key);
		return EINVAL;
	}

	m->lock_count--;

	if (m->lock_count == 0U) {
		thread = z_unpend_first_thread(&m->wait_q);
		if (thread) {
			m->owner = (pthread_t)thread;
			m->lock_count++;
			z_ready_thread(thread);
			arch_thread_return_value_set(thread, 0);
			z_reschedule_irqlock(key);
			return 0;
		}
		m->owner = NULL;

	}
	irq_unlock(key);
	return 0;
}

/**
 * @brief Destroy POSIX mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutex_destroy(pthread_mutex_t *m)
{
	ARG_UNUSED(m);
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
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
	*type = attr->type;
	return 0;
}

/**
 * @brief Set type attribute for mutex.
 *
 * See IEEE 1003.1
 */
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	int retc = EINVAL;

	if ((type == PTHREAD_MUTEX_NORMAL) ||
	    (type == PTHREAD_MUTEX_RECURSIVE) ||
	    (type == PTHREAD_MUTEX_ERRORCHECK)) {
		attr->type = type;
		retc = 0;
	}

	return retc;
}
