/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/wait_q.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/bitarray.h>

#include "posix_internal.h"

int64_t timespec_to_timeoutms(const struct timespec *abstime);

static struct k_condvar posix_cond_pool[CONFIG_MAX_PTHREAD_COND_COUNT];
SYS_BITARRAY_DEFINE_STATIC(posix_cond_bitarray, CONFIG_MAX_PTHREAD_COND_COUNT);

/*
 * We reserve the MSB to mark a pthread_cond_t as initialized (from the
 * perspective of the application). With a linear space, this means that
 * the theoretical pthread_cond_t range is [0,2147483647].
 */
BUILD_ASSERT(CONFIG_MAX_PTHREAD_COND_COUNT < PTHREAD_OBJ_MASK_INIT,
	     "CONFIG_MAX_PTHREAD_COND_COUNT is too high");

static inline size_t posix_cond_to_offset(struct k_condvar *cv)
{
	return cv - posix_cond_pool;
}

static inline size_t to_posix_cond_idx(pthread_cond_t cond)
{
	return mark_pthread_obj_uninitialized(cond);
}

struct k_condvar *get_posix_cond(pthread_cond_t cond)
{
	int actually_initialized;
	size_t bit = to_posix_cond_idx(cond);

	/* if the provided cond does not claim to be initialized, its invalid */
	if (!is_pthread_obj_initialized(cond)) {
		return NULL;
	}

	/* Mask off the MSB to get the actual bit index */
	if (sys_bitarray_test_bit(&posix_cond_bitarray, bit, &actually_initialized) < 0) {
		return NULL;
	}

	if (actually_initialized == 0) {
		/* The cond claims to be initialized but is actually not */
		return NULL;
	}

	return &posix_cond_pool[bit];
}

struct k_condvar *to_posix_cond(pthread_cond_t *cvar)
{
	size_t bit;
	struct k_condvar *cv;

	if (*cvar != PTHREAD_COND_INITIALIZER) {
		return get_posix_cond(*cvar);
	}

	/* Try and automatically associate a posix_cond */
	if (sys_bitarray_alloc(&posix_cond_bitarray, 1, &bit) < 0) {
		/* No conds left to allocate */
		return NULL;
	}

	/* Record the associated posix_cond in mu and mark as initialized */
	*cvar = mark_pthread_obj_initialized(bit);
	cv = &posix_cond_pool[bit];

	/* Initialize the condition variable here */
	z_waitq_init(&cv->wait_q);

	return cv;
}

static int cond_wait(pthread_cond_t *cond, pthread_mutex_t *mu, k_timeout_t timeout)
{
	int ret;
	k_spinlock_key_t key;
	struct k_condvar *cv;
	struct k_mutex *m;

	key = k_spin_lock(&z_pthread_spinlock);
	m = to_posix_mutex(mu);
	if (m == NULL) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return EINVAL;
	}

	cv = to_posix_cond(cond);
	if (cv == NULL) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return EINVAL;
	}

	__ASSERT_NO_MSG(m->lock_count == 1U);
	m->lock_count = 0U;
	m->owner = NULL;
	_ready_one_thread(&m->wait_q);
	ret = z_sched_wait(&z_pthread_spinlock, key, &cv->wait_q, timeout, NULL);

	/* FIXME: this extra lock (and the potential context switch it
	 * can cause) could be optimized out.  At the point of the
	 * signal/broadcast, it's possible to detect whether or not we
	 * will be swapping back to this particular thread and lock it
	 * (i.e. leave the lock variable unchanged) on our behalf.
	 * But that requires putting scheduler intelligence into this
	 * higher level abstraction and is probably not worth it.
	 */
	pthread_mutex_lock(mu);

	return ret == -EAGAIN ? ETIMEDOUT : ret;
}

int pthread_cond_signal(pthread_cond_t *cvar)
{
	k_spinlock_key_t key;
	struct k_condvar *cv;

	key = k_spin_lock(&z_pthread_spinlock);

	cv = to_posix_cond(cvar);
	if (cv == NULL) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return EINVAL;
	}

	k_spin_unlock(&z_pthread_spinlock, key);

	z_sched_wake(&cv->wait_q, 0, NULL);

	return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cvar)
{
	k_spinlock_key_t key;
	struct k_condvar *cv;

	key = k_spin_lock(&z_pthread_spinlock);

	cv = to_posix_cond(cvar);
	if (cv == NULL) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return EINVAL;
	}

	k_spin_unlock(&z_pthread_spinlock, key);

	z_sched_wake_all(&cv->wait_q, 0, NULL);
	return 0;
}

int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *mut)
{
	return cond_wait(cv, mut, K_FOREVER);
}

int pthread_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *mut, const struct timespec *abstime)
{
	int32_t timeout = (int32_t)timespec_to_timeoutms(abstime);
	return cond_wait(cv, mut, K_MSEC(timeout));
}

int pthread_cond_init(pthread_cond_t *cvar, const pthread_condattr_t *att)
{
	k_spinlock_key_t key;
	struct k_condvar *cv;

	ARG_UNUSED(att);
	*cvar = PTHREAD_COND_INITIALIZER;

	key = k_spin_lock(&z_pthread_spinlock);

	cv = to_posix_cond(cvar);
	if (cv == NULL) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return EINVAL;
	}

	k_spin_unlock(&z_pthread_spinlock, key);

	return 0;
}

int pthread_cond_destroy(pthread_cond_t *cvar)
{
	__unused int rc;
	k_spinlock_key_t key;
	struct k_condvar *cv;
	pthread_cond_t c = *cvar;
	size_t bit = to_posix_cond_idx(c);

	key = k_spin_lock(&z_pthread_spinlock);

	cv = get_posix_cond(c);
	if (cv == NULL) {
		k_spin_unlock(&z_pthread_spinlock, key);
		return EINVAL;
	}

	rc = sys_bitarray_free(&posix_cond_bitarray, 1, bit);
	__ASSERT(rc == 0, "failed to free bit %zu", bit);

	k_spin_unlock(&z_pthread_spinlock, key);

	return 0;
}

static int pthread_cond_pool_init(void)
{
	int err;
	size_t i;

	for (i = 0; i < CONFIG_MAX_PTHREAD_COND_COUNT; ++i) {
		err = k_condvar_init(&posix_cond_pool[i]);
		__ASSERT_NO_MSG(err == 0);
	}

	return 0;
}
SYS_INIT(pthread_cond_pool_init, PRE_KERNEL_1, 0);
