/*
 * Copyright (c) 2025 Cadence Design Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_MULTITHREADING

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <sys/reent.h>

typedef struct k_mutex *_Rmtx;

void _Mtxinit(_Rmtx *mtx)
{
	*mtx = malloc(sizeof(struct k_mutex));
	if (!*mtx) {
		k_panic();
	}
	k_mutex_init(*mtx);
}

void _Mtxdst(_Rmtx *mtx)
{
	if ((mtx != NULL) && (*mtx != NULL)) {
		free(*mtx);
	}
}

void _Mtxlock(_Rmtx *mtx)
{
	if ((mtx != NULL) && (*mtx != NULL)) {
		k_mutex_lock(*mtx, K_FOREVER);
	}
}

void _Mtxunlock(_Rmtx *mtx)
{
	if ((mtx != NULL) && (*mtx != NULL)) {
		k_mutex_unlock(*mtx);
	}
}

extern struct _reent *_reent_ptr;
void _xclib_update_reent_ptr(struct k_thread *thread)
{
	_reent_ptr = &thread->arch.reent;
}

#endif /* CONFIG_MULTITHREADING */
