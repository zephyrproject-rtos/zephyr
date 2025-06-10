/*
 * Copyright (c) 2025 Cadence Design Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <sys/reent.h>

#if !defined(CONFIG_XTENSA_LIBC_DYNAMIC_REENT)
void _init_reent_bss(void);
#endif

void xtensa_xclib_init(void)
{
#if !defined(CONFIG_XTENSA_LIBC_DYNAMIC_REENT)
	_init_reent_bss();
#endif
}

int32_t _xclib_use_mt = 1; /* Enables xclib multithread support */

typedef struct k_mutex *_Rmtx;

/* Number of Locks needed by _Initlocks() without calling malloc()
 * which would infinitely call _Initlocks() recursively.
 */
#define NLOCKS (_MAX_LOCK + 1)

static struct k_mutex _Mtxinit_mtx[NLOCKS];

void _Mtxinit(_Rmtx *mtx)
{
	static atomic_t i;
	int idx = atomic_inc(&i);

	if (idx < NLOCKS) {
		*mtx = &_Mtxinit_mtx[idx];
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

#if defined(CONFIG_XTENSA_LIBC_DYNAMIC_REENT)
struct _reent *__getreent(void)
{
	return &k_current_get()->arch.reent;
}
#endif
