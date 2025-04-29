/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_clock.h"
#include "posix_internal.h"

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/bitarray.h>

LOG_MODULE_REGISTER(pthread_cond, CONFIG_PTHREAD_COND_LOG_LEVEL);

static __pinned_bss struct posix_cond posix_cond_pool[CONFIG_MAX_PTHREAD_COND_COUNT];

SYS_BITARRAY_DEFINE_STATIC(posix_cond_bitarray, CONFIG_MAX_PTHREAD_COND_COUNT);

BUILD_ASSERT(sizeof(struct posix_condattr) <= sizeof(pthread_condattr_t),
	     "posix_condattr is too large");

/*
 * We reserve the MSB to mark a pthread_cond_t as initialized (from the
 * perspective of the application). With a linear space, this means that
 * the theoretical pthread_cond_t range is [0,2147483647].
 */
BUILD_ASSERT(CONFIG_MAX_PTHREAD_COND_COUNT < PTHREAD_OBJ_MASK_INIT,
	     "CONFIG_MAX_PTHREAD_COND_COUNT is too high");

static inline size_t posix_cond_to_offset(struct posix_cond *cv)
{
	return cv - posix_cond_pool;
}

static inline size_t to_posix_cond_idx(pthread_cond_t cond)
{
	return mark_pthread_obj_uninitialized(cond);
}

static struct posix_cond *get_posix_cond(pthread_cond_t cond)
{
	int actually_initialized;
	size_t bit = to_posix_cond_idx(cond);

	/* if the provided cond does not claim to be initialized, its invalid */
	if (!is_pthread_obj_initialized(cond)) {
		LOG_DBG("Cond is uninitialized (%x)", cond);
		return NULL;
	}

	/* Mask off the MSB to get the actual bit index */
	if (sys_bitarray_test_bit(&posix_cond_bitarray, bit, &actually_initialized) < 0) {
		LOG_DBG("Cond is invalid (%x)", cond);
		return NULL;
	}

	if (actually_initialized == 0) {
		/* The cond claims to be initialized but is actually not */
		LOG_DBG("Cond claims to be initialized (%x)", cond);
		return NULL;
	}

	return &posix_cond_pool[bit];
}

static struct posix_cond *to_posix_cond(pthread_cond_t *cvar)
{
	size_t bit;
	struct posix_cond *cv;

	if (*cvar != PTHREAD_COND_INITIALIZER) {
		return get_posix_cond(*cvar);
	}

	/* Try and automatically associate a posix_cond */
	if (sys_bitarray_alloc(&posix_cond_bitarray, 1, &bit) < 0) {
		/* No conds left to allocate */
		LOG_DBG("Unable to allocate pthread_cond_t");
		return NULL;
	}

	/* Record the associated posix_cond in mu and mark as initialized */
	*cvar = mark_pthread_obj_initialized(bit);
	cv = &posix_cond_pool[bit];
	(void)pthread_condattr_init((pthread_condattr_t *)&cv->attr);

	return cv;
}

static int cond_wait(pthread_cond_t *cond, pthread_mutex_t *mu, const struct timespec *abstime)
{
	int ret;
	struct k_mutex *m;
	struct posix_cond *cv;
	k_timeout_t timeout = K_FOREVER;

	m = to_posix_mutex(mu);
	cv = to_posix_cond(cond);
	if (cv == NULL || m == NULL) {
		return EINVAL;
	}

	if (abstime != NULL) {
		timeout = K_MSEC(timespec_to_timeoutms(cv->attr.clock, abstime));
	}

	LOG_DBG("Waiting on cond %p with timeout %llx", cv, timeout.ticks);
	ret = k_condvar_wait(&cv->condvar, m, timeout);
	if (ret == -EAGAIN) {
		LOG_DBG("Timeout waiting on cond %p", cv);
		ret = ETIMEDOUT;
	} else if (ret < 0) {
		LOG_DBG("k_condvar_wait() failed: %d", ret);
		ret = -ret;
	} else {
		__ASSERT_NO_MSG(ret == 0);
		LOG_DBG("Cond %p received signal", cv);
	}

	return ret;
}

int pthread_cond_signal(pthread_cond_t *cvar)
{
	int ret;
	struct posix_cond *cv;

	cv = to_posix_cond(cvar);
	if (cv == NULL) {
		return EINVAL;
	}

	LOG_DBG("Signaling cond %p", cv);
	ret = k_condvar_signal(&cv->condvar);
	if (ret < 0) {
		LOG_DBG("k_condvar_signal() failed: %d", ret);
		return -ret;
	}

	__ASSERT_NO_MSG(ret == 0);

	return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cvar)
{
	int ret;
	struct posix_cond *cv;

	cv = get_posix_cond(*cvar);
	if (cv == NULL) {
		return EINVAL;
	}

	LOG_DBG("Broadcasting on cond %p", cv);
	ret = k_condvar_broadcast(&cv->condvar);
	if (ret < 0) {
		LOG_DBG("k_condvar_broadcast() failed: %d", ret);
		return -ret;
	}

	__ASSERT_NO_MSG(ret >= 0);

	return 0;
}

int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *mut)
{
	return cond_wait(cv, mut, NULL);
}

int pthread_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *mut, const struct timespec *abstime)
{
	if ((abstime == NULL) || !timespec_is_valid(abstime)) {
		LOG_DBG("%s is invalid", "abstime");
		return EINVAL;
	}

	return cond_wait(cv, mut, abstime);
}

int pthread_cond_init(pthread_cond_t *cvar, const pthread_condattr_t *att)
{
	struct posix_cond *cv;
	struct posix_condattr *attr = (struct posix_condattr *)att;

	*cvar = PTHREAD_COND_INITIALIZER;
	cv = to_posix_cond(cvar);
	if (cv == NULL) {
		return ENOMEM;
	}

	if (attr != NULL) {
		if (!attr->initialized) {
			return EINVAL;
		}

		(void)pthread_condattr_destroy((pthread_condattr_t *)&cv->attr);
		cv->attr = *attr;
	}

	LOG_DBG("Initialized cond %p", cv);

	return 0;
}

int pthread_cond_destroy(pthread_cond_t *cvar)
{
	int err;
	size_t bit;
	struct posix_cond *cv;

	cv = get_posix_cond(*cvar);
	if (cv == NULL) {
		return EINVAL;
	}

	bit = posix_cond_to_offset(cv);
	err = sys_bitarray_free(&posix_cond_bitarray, 1, bit);
	__ASSERT_NO_MSG(err == 0);

	*cvar = -1;

	LOG_DBG("Destroyed cond %p", cv);

	return 0;
}

__boot_func
static int pthread_cond_pool_init(void)
{
	int err;
	size_t i;

	for (i = 0; i < CONFIG_MAX_PTHREAD_COND_COUNT; ++i) {
		err = k_condvar_init(&posix_cond_pool[i].condvar);
		__ASSERT_NO_MSG(err == 0);
	}

	return 0;
}

int pthread_condattr_init(pthread_condattr_t *att)
{
	struct posix_condattr *const attr = (struct posix_condattr *)att;

	if (att == NULL) {
		return EINVAL;
	}
	if (attr->initialized) {
		LOG_DBG("%s %s initialized", "attribute", "already");
		return EINVAL;
	}

	attr->clock = CLOCK_REALTIME;
	attr->initialized = true;

	return 0;
}

int pthread_condattr_destroy(pthread_condattr_t *att)
{
	struct posix_condattr *const attr = (struct posix_condattr *)att;

	if ((attr == NULL) || !attr->initialized) {
		LOG_DBG("%s %s initialized", "attribute", "not");
		return EINVAL;
	}

	*attr = (struct posix_condattr){0};

	return 0;
}

int pthread_condattr_getclock(const pthread_condattr_t *ZRESTRICT att,
		clockid_t *ZRESTRICT clock_id)
{
	struct posix_condattr *const attr = (struct posix_condattr *)att;

	if ((attr == NULL) || !attr->initialized) {
		LOG_DBG("%s %s initialized", "attribute", "not");
		return EINVAL;
	}

	*clock_id = attr->clock;

	return 0;
}

int pthread_condattr_setclock(pthread_condattr_t *att, clockid_t clock_id)
{
	struct posix_condattr *const attr = (struct posix_condattr *)att;

	if (clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC) {
		return -EINVAL;
	}

	if ((attr == NULL) || !attr->initialized) {
		LOG_DBG("%s %s initialized", "attribute", "not");
		return EINVAL;
	}

	attr->clock = clock_id;

	return 0;
}
SYS_INIT(pthread_cond_pool_init, PRE_KERNEL_1, 0);
