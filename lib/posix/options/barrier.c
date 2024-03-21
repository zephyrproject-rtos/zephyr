/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_internal.h"

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/bitarray.h>

struct posix_barrier {
	struct k_mutex mutex;
	struct k_condvar cond;
	uint32_t max;
	uint32_t count;
};

static struct posix_barrier posix_barrier_pool[CONFIG_MAX_PTHREAD_BARRIER_COUNT];
SYS_BITARRAY_DEFINE_STATIC(posix_barrier_bitarray, CONFIG_MAX_PTHREAD_BARRIER_COUNT);

/*
 * We reserve the MSB to mark a pthread_barrier_t as initialized (from the
 * perspective of the application). With a linear space, this means that
 * the theoretical pthread_barrier_t range is [0,2147483647].
 */
BUILD_ASSERT(CONFIG_MAX_PTHREAD_BARRIER_COUNT < PTHREAD_OBJ_MASK_INIT,
	     "CONFIG_MAX_PTHREAD_BARRIER_COUNT is too high");

static inline size_t posix_barrier_to_offset(struct posix_barrier *bar)
{
	return bar - posix_barrier_pool;
}

static inline size_t to_posix_barrier_idx(pthread_barrier_t b)
{
	return mark_pthread_obj_uninitialized(b);
}

struct posix_barrier *get_posix_barrier(pthread_barrier_t b)
{
	int actually_initialized;
	size_t bit = to_posix_barrier_idx(b);

	/* if the provided barrier does not claim to be initialized, its invalid */
	if (!is_pthread_obj_initialized(b)) {
		return NULL;
	}

	/* Mask off the MSB to get the actual bit index */
	if (sys_bitarray_test_bit(&posix_barrier_bitarray, bit, &actually_initialized) < 0) {
		return NULL;
	}

	if (actually_initialized == 0) {
		/* The barrier claims to be initialized but is actually not */
		return NULL;
	}

	return &posix_barrier_pool[bit];
}

int pthread_barrier_wait(pthread_barrier_t *b)
{
	int ret;
	int err;
	pthread_barrier_t bb = *b;
	struct posix_barrier *bar;

	bar = get_posix_barrier(bb);
	if (bar == NULL) {
		return EINVAL;
	}

	err = k_mutex_lock(&bar->mutex, K_FOREVER);
	__ASSERT_NO_MSG(err == 0);

	++bar->count;

	if (bar->count == bar->max) {
		bar->count = 0;
		ret = PTHREAD_BARRIER_SERIAL_THREAD;

		goto unlock;
	}

	while (bar->count != 0) {
		err = k_condvar_wait(&bar->cond, &bar->mutex, K_FOREVER);
		__ASSERT_NO_MSG(err == 0);
		/* Note: count is reset to zero by the serialized thread */
	}

	ret = 0;

unlock:
	err = k_condvar_signal(&bar->cond);
	__ASSERT_NO_MSG(err == 0);
	err = k_mutex_unlock(&bar->mutex);
	__ASSERT_NO_MSG(err == 0);

	return ret;
}

int pthread_barrier_init(pthread_barrier_t *b, const pthread_barrierattr_t *attr,
			 unsigned int count)
{
	size_t bit;
	struct posix_barrier *bar;

	if (count == 0) {
		return EINVAL;
	}

	if (sys_bitarray_alloc(&posix_barrier_bitarray, 1, &bit) < 0) {
		return ENOMEM;
	}

	bar = &posix_barrier_pool[bit];
	bar->max = count;
	bar->count = 0;

	*b = mark_pthread_obj_initialized(bit);

	return 0;
}

int pthread_barrier_destroy(pthread_barrier_t *b)
{
	int err;
	size_t bit;
	struct posix_barrier *bar;

	bar = get_posix_barrier(*b);
	if (bar == NULL) {
		return EINVAL;
	}

	err = k_mutex_lock(&bar->mutex, K_FOREVER);
	if (err < 0) {
		return -err;
	}
	__ASSERT_NO_MSG(err == 0);

	/* An implementation may use this function to set barrier to an invalid value */
	bar->max = 0;
	bar->count = 0;

	bit = posix_barrier_to_offset(bar);
	err = sys_bitarray_free(&posix_barrier_bitarray, 1, bit);
	__ASSERT_NO_MSG(err == 0);

	err = k_condvar_broadcast(&bar->cond);
	__ASSERT_NO_MSG(err == 0);

	err = k_mutex_unlock(&bar->mutex);
	__ASSERT_NO_MSG(err == 0);

	return 0;
}

int pthread_barrierattr_init(pthread_barrierattr_t *attr)
{
	__ASSERT_NO_MSG(attr != NULL);

	attr->pshared = PTHREAD_PROCESS_PRIVATE;

	return 0;
}

int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr, int pshared)
{
	if (pshared != PTHREAD_PROCESS_PRIVATE && pshared != PTHREAD_PROCESS_PUBLIC) {
		return -EINVAL;
	}

	attr->pshared = pshared;
	return 0;
}

int pthread_barrierattr_getpshared(const pthread_barrierattr_t *restrict attr,
				   int *restrict pshared)
{
	*pshared = attr->pshared;

	return 0;
}

int pthread_barrierattr_destroy(pthread_barrierattr_t *attr)
{
	ARG_UNUSED(attr);

	return 0;
}

static int pthread_barrier_pool_init(void)
{
	int err;
	size_t i;

	for (i = 0; i < CONFIG_MAX_PTHREAD_BARRIER_COUNT; ++i) {
		err = k_mutex_init(&posix_barrier_pool[i].mutex);
		__ASSERT_NO_MSG(err == 0);
		err = k_condvar_init(&posix_barrier_pool[i].cond);
		__ASSERT_NO_MSG(err == 0);
	}

	return 0;
}
SYS_INIT(pthread_barrier_pool_init, PRE_KERNEL_1, 0);
