/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <threads.h>

#include <zephyr/posix/pthread.h>

int cnd_broadcast(cnd_t *cond)
{
	switch (pthread_cond_broadcast(cond)) {
	case 0:
		return thrd_success;
	default:
		return thrd_error;
	}
}

void cnd_destroy(cnd_t *cond)
{
	(void)pthread_cond_destroy(cond);
}

int cnd_init(cnd_t *cond)
{
	switch (pthread_cond_init(cond, NULL)) {
	case 0:
		return thrd_success;
	case ENOMEM:
		return thrd_nomem;
	default:
		return thrd_error;
	}
}

int cnd_signal(cnd_t *cond)
{
	switch (pthread_cond_signal(cond)) {
	case 0:
		return thrd_success;
	case ENOMEM:
		return thrd_nomem;
	default:
		return thrd_error;
	}
}

int cnd_timedwait(cnd_t *restrict cond, mtx_t *restrict mtx, const struct timespec *restrict ts)
{
	switch (pthread_cond_timedwait(cond, mtx, ts)) {
	case 0:
		return thrd_success;
	case ETIMEDOUT:
		return thrd_timedout;
	default:
		return thrd_error;
	}
}

int cnd_wait(cnd_t *cond, mtx_t *mtx)
{
	switch (pthread_cond_wait(cond, mtx)) {
	case 0:
		return thrd_success;
	default:
		return thrd_error;
	}
}
