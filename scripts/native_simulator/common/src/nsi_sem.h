/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_NSI_SEM_H
#define NSI_COMMON_SRC_NSI_SEM_H

#include <stdint.h>

#include "nsi_utils.h"

/*
 * Counting semaphore wrappers. They follow the sem_t convention: 0 on success,
 * -1 with errno set on failure, so callers can retry on EINTR as usual.
 */
#ifdef __APPLE__
/* Darwin has POSIX named semaphores only, build one out of pthread primitives */
#include <errno.h>
#include <pthread.h>

typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	uint32_t count;
} nsi_sem_t;

NSI_INLINE int nsi_sem_fail(int err)
{
	errno = err;
	return -1;
}

NSI_INLINE int nsi_sem_init(nsi_sem_t *sem, unsigned int value)
{
	int ret;

	ret = pthread_mutex_init(&sem->mutex, NULL);
	if (ret != 0) {
		return nsi_sem_fail(ret);
	}

	ret = pthread_cond_init(&sem->cond, NULL);
	if (ret != 0) {
		(void)pthread_mutex_destroy(&sem->mutex);
		return nsi_sem_fail(ret);
	}

	sem->count = value;

	return 0;
}

NSI_INLINE int nsi_sem_post(nsi_sem_t *sem)
{
	int ret;
	int unlock_ret;

	ret = pthread_mutex_lock(&sem->mutex);
	if (ret != 0) {
		return nsi_sem_fail(ret);
	}

	if (sem->count == UINT32_MAX) {
		(void)pthread_mutex_unlock(&sem->mutex);
		return nsi_sem_fail(EOVERFLOW);
	}

	sem->count++;
	ret = pthread_cond_signal(&sem->cond);
	unlock_ret = pthread_mutex_unlock(&sem->mutex);

	if (ret != 0) {
		return nsi_sem_fail(ret);
	}
	if (unlock_ret != 0) {
		return nsi_sem_fail(unlock_ret);
	}

	return 0;
}

NSI_INLINE int nsi_sem_wait(nsi_sem_t *sem)
{
	int ret;
	int unlock_ret;

	ret = pthread_mutex_lock(&sem->mutex);
	if (ret != 0) {
		return nsi_sem_fail(ret);
	}

	while (sem->count == 0U) {
		ret = pthread_cond_wait(&sem->cond, &sem->mutex);
		if (ret != 0) {
			unlock_ret = pthread_mutex_unlock(&sem->mutex);
			if (unlock_ret != 0) {
				return nsi_sem_fail(unlock_ret);
			}

			return nsi_sem_fail(ret);
		}
	}

	sem->count--;

	unlock_ret = pthread_mutex_unlock(&sem->mutex);
	if (unlock_ret != 0) {
		return nsi_sem_fail(unlock_ret);
	}

	return 0;
}

#else
#include <semaphore.h>

typedef sem_t nsi_sem_t;

NSI_INLINE int nsi_sem_init(nsi_sem_t *sem, unsigned int value)
{
	return sem_init(sem, 0, value);
}

NSI_INLINE int nsi_sem_post(nsi_sem_t *sem)
{
	return sem_post(sem);
}

NSI_INLINE int nsi_sem_wait(nsi_sem_t *sem)
{
	return sem_wait(sem);
}
#endif

#endif /* NSI_COMMON_SRC_NSI_SEM_H */
