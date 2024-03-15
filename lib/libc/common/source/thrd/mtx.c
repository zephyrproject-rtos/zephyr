/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <threads.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/pthread.h>

int mtx_init(mtx_t *mutex, int type)
{
	int ret;
	pthread_mutexattr_t attr;
	pthread_mutexattr_t *attrp = NULL;

	switch (type) {
	case mtx_plain:
	case mtx_timed:
		break;
	case mtx_plain | mtx_recursive:
	case mtx_timed | mtx_recursive:
		attrp = &attr;
		ret = pthread_mutexattr_init(attrp);
		__ASSERT_NO_MSG(ret == 0);

		ret = pthread_mutexattr_settype(attrp, PTHREAD_MUTEX_RECURSIVE);
		__ASSERT_NO_MSG(ret == 0);
		break;
	default:
		return thrd_error;
	}

	switch (pthread_mutex_init(mutex, attrp)) {
	case 0:
		ret = thrd_success;
		break;
	default:
		ret = thrd_error;
		break;
	}

	if (attrp != NULL) {
		(void)pthread_mutexattr_destroy(attrp);
	}

	return ret;
}

void mtx_destroy(mtx_t *mutex)
{
	(void)pthread_mutex_destroy(mutex);
}

int mtx_lock(mtx_t *mutex)
{
	switch (pthread_mutex_lock(mutex)) {
	case 0:
		return thrd_success;
	default:
		return thrd_error;
	}
}

int mtx_timedlock(mtx_t *restrict mutex, const struct timespec *restrict time_point)
{
	switch (pthread_mutex_timedlock(mutex, time_point)) {
	case 0:
		return thrd_success;
	case ETIMEDOUT:
		return thrd_timedout;
	default:
		return thrd_error;
	}
}

int mtx_trylock(mtx_t *mutex)
{
	switch (pthread_mutex_trylock(mutex)) {
	case 0:
		return thrd_success;
	case EBUSY:
		return thrd_busy;
	default:
		return thrd_error;
	}
}

int mtx_unlock(mtx_t *mutex)
{
	switch (pthread_mutex_unlock(mutex)) {
	case 0:
		return thrd_success;
	default:
		return thrd_error;
	}
}
