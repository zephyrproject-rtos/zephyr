/*
 * Copyright (c) 2025 Cadence Design Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <sys/reent.h>

int32_t _xclib_use_mt = 1; /* Enables xclib multithread support */

typedef struct k_mutex *_Rmtx;

/* Number of Locks needed by _Initlocks() without calling malloc()
 * which would infinitely call _Initlocks() recursively.
 */
#define NLOCKS (_MAX_LOCK + 1)

static struct k_mutex _Mtxinit_mtx[NLOCKS];

void _Mtxinit(_Rmtx *mtx)
{
	static int i; /* static variable initially 0 */

	if (i < NLOCKS) {
		*mtx = &_Mtxinit_mtx[i];
		++i;
	} else {
		*mtx = malloc(sizeof(struct k_mutex));
	}
	if (!*mtx) {
		k_panic();
	}
	k_mutex_init(*mtx);
}

void _Mtxdst(_Rmtx *mtx)
{
	if (mtx != NULL && *mtx != NULL &&
		(*mtx > &_Mtxinit_mtx[NLOCKS] || *mtx < &_Mtxinit_mtx[0])) {
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

#if defined(__DYNAMIC_REENT__)
struct _reent *__getreent(void)
{
	return &k_current_get()->arch.reent;
}
#else
#error __DYNAMIC_REENT__ support missing
#endif
