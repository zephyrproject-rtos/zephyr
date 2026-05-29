/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <threads.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/pthread.h>

int tss_create(tss_t *key, tss_dtor_t destructor)
{
	switch (pthread_key_create(key, destructor)) {
	case 0:
		return thrd_success;
	case EAGAIN:
		return thrd_busy;
	case ENOMEM:
		return thrd_nomem;
	default:
		return thrd_error;
	}
}

void *tss_get(tss_t key)
{
	return pthread_getspecific(key);
}

int tss_set(tss_t key, void *val)
{
	switch (pthread_setspecific(key, val)) {
	case 0:
		return thrd_success;
	case ENOMEM:
		return thrd_nomem;
	default:
		return thrd_error;
	}
}

void tss_delete(tss_t key)
{
	(void)pthread_key_delete(key);
}
