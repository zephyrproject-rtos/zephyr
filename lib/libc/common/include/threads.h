/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_COMMON_INCLUDE_THREADS_H_
#define ZEPHYR_LIB_LIBC_COMMON_INCLUDE_THREADS_H_

#include <time.h>

#include <machine/_threads.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*thrd_start_t)(void *arg);

enum {
	thrd_success,
#define thrd_success thrd_success
	thrd_nomem,
#define thrd_nomem thrd_nomem
	thrd_timedout,
#define thrd_timedout thrd_timedout
	thrd_busy,
#define thrd_busy thrd_busy
	thrd_error,
#define thrd_error thrd_error
};

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
int thrd_equal(thrd_t lhs, thrd_t rhs);
thrd_t thrd_current(void);
int thrd_sleep(const struct timespec *duration, struct timespec *remaining);
void thrd_yield(void);
_Noreturn void thrd_exit(int res);
int thrd_detach(thrd_t thr);
int thrd_join(thrd_t thr, int *res);

enum {
	mtx_plain,
#define mtx_plain mtx_plain
	mtx_timed,
#define mtx_timed mtx_timed
	mtx_recursive,
#define mtx_recursive mtx_recursive
};

int mtx_init(mtx_t *mutex, int type);
void mtx_destroy(mtx_t *mutex);
int mtx_lock(mtx_t *mutex);
int mtx_timedlock(mtx_t *ZRESTRICT mutex, const struct timespec *ZRESTRICT time_point);
int mtx_trylock(mtx_t *mutex);
int mtx_unlock(mtx_t *mutex);

int cnd_init(cnd_t *cond);
int cnd_wait(cnd_t *cond, mtx_t *mtx);
int cnd_timedwait(cnd_t *ZRESTRICT cond, mtx_t *ZRESTRICT mtx, const struct timespec *ZRESTRICT ts);
int cnd_signal(cnd_t *cond);
int cnd_broadcast(cnd_t *cond);
void cnd_destroy(cnd_t *cond);

#ifndef thread_local
#define	thread_local _Thread_local
#endif

typedef void (*tss_dtor_t)(void *val);

int tss_create(tss_t *key, tss_dtor_t destructor);
void *tss_get(tss_t key);
int tss_set(tss_t key, void *val);
void tss_delete(tss_t key);

void call_once(once_flag *flag, void (*func)(void));

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_COMMON_INCLUDE_THREADS_H_ */
