/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_NSI_SEM_H
#define NSI_COMMON_SRC_NSI_SEM_H

#include <stdint.h>

#ifdef __APPLE__
#include <errno.h>
#include <pthread.h>

typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	uint32_t count;
} nsi_sem_t;

static inline int nsi_sem_init(nsi_sem_t *sem, unsigned int value)
{
	int ret;

	ret = pthread_mutex_init(&sem->mutex, NULL);
	if (ret != 0) {
		return ret;
	}

	ret = pthread_cond_init(&sem->cond, NULL);
	if (ret != 0) {
		(void)pthread_mutex_destroy(&sem->mutex);
		return ret;
	}

	sem->count = value;

	return 0;
}

static inline int nsi_sem_post(nsi_sem_t *sem)
{
	int ret;
	int unlock_ret;

	ret = pthread_mutex_lock(&sem->mutex);
	if (ret != 0) {
		return ret;
	}

	if (sem->count == UINT32_MAX) {
		(void)pthread_mutex_unlock(&sem->mutex);
		return EOVERFLOW;
	}

	sem->count++;
	ret = pthread_cond_signal(&sem->cond);
	unlock_ret = pthread_mutex_unlock(&sem->mutex);

	if (ret != 0) {
		return ret;
	}

	return unlock_ret;
}

static inline int nsi_sem_wait(nsi_sem_t *sem)
{
	int ret;
	int unlock_ret;

	ret = pthread_mutex_lock(&sem->mutex);
	if (ret != 0) {
		return ret;
	}

	while (sem->count == 0U) {
		ret = pthread_cond_wait(&sem->cond, &sem->mutex);
		if (ret != 0) {
			unlock_ret = pthread_mutex_unlock(&sem->mutex);
			if (unlock_ret != 0) {
				return unlock_ret;
			}

			return ret;
		}
	}

	sem->count--;

	return pthread_mutex_unlock(&sem->mutex);
}

#else
#include <semaphore.h>

typedef sem_t nsi_sem_t;

static inline int nsi_sem_init(nsi_sem_t *sem, unsigned int value)
{
	return sem_init(sem, 0, value);
}

static inline int nsi_sem_post(nsi_sem_t *sem)
{
	return sem_post(sem);
}

static inline int nsi_sem_wait(nsi_sem_t *sem)
{
	return sem_wait(sem);
}
#endif

#endif /* NSI_COMMON_SRC_NSI_SEM_H */
