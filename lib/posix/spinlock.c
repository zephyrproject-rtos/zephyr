/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_internal.h"

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/bitarray.h>

union _spinlock_storage {
	struct k_spinlock lock;
	uint8_t byte;
};
#if !defined(CONFIG_CPP) && !defined(CONFIG_SMP) && !defined(CONFIG_SPIN_VALIDATE)
BUILD_ASSERT(sizeof(struct k_spinlock) == 0,
	     "please remove the _spinlock_storage workaround if, at some point, k_spinlock is no "
	     "longer zero bytes when CONFIG_SMP=n && CONFIG_SPIN_VALIDATE=n");
#endif

static union _spinlock_storage posix_spinlock_pool[CONFIG_MAX_PTHREAD_SPINLOCK_COUNT];
static k_spinlock_key_t posix_spinlock_key[CONFIG_MAX_PTHREAD_SPINLOCK_COUNT];
SYS_BITARRAY_DEFINE_STATIC(posix_spinlock_bitarray, CONFIG_MAX_PTHREAD_SPINLOCK_COUNT);

/*
 * We reserve the MSB to mark a pthread_spinlock_t as initialized (from the
 * perspective of the application). With a linear space, this means that
 * the theoretical pthread_spinlock_t range is [0,2147483647].
 */
BUILD_ASSERT(CONFIG_MAX_PTHREAD_SPINLOCK_COUNT < PTHREAD_OBJ_MASK_INIT,
	"CONFIG_MAX_PTHREAD_SPINLOCK_COUNT is too high");

static inline size_t posix_spinlock_to_offset(struct k_spinlock *l)
{
	return (union _spinlock_storage *)l - posix_spinlock_pool;
}

static inline size_t to_posix_spinlock_idx(pthread_spinlock_t lock)
{
	return mark_pthread_obj_uninitialized(lock);
}

static struct k_spinlock *get_posix_spinlock(pthread_spinlock_t *lock)
{
	size_t bit;
	int actually_initialized;

	if (lock == NULL) {
		return NULL;
	}

	/* if the provided spinlock does not claim to be initialized, its invalid */
	bit = to_posix_spinlock_idx(*lock);
	if (!is_pthread_obj_initialized(*lock)) {
		return NULL;
	}

	/* Mask off the MSB to get the actual bit index */
	if (sys_bitarray_test_bit(&posix_spinlock_bitarray, bit, &actually_initialized) < 0) {
		return NULL;
	}

	if (actually_initialized == 0) {
		/* The spinlock claims to be initialized but is actually not */
		return NULL;
	}

	return (struct k_spinlock *)&posix_spinlock_pool[bit];
}

int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
	int ret;
	size_t bit;

	if (lock == NULL ||
	    !(pshared == PTHREAD_PROCESS_PRIVATE || pshared == PTHREAD_PROCESS_SHARED)) {
		/* not specified as part of POSIX but this is the Linux behavior */
		return EINVAL;
	}

	ret = sys_bitarray_alloc(&posix_spinlock_bitarray, 1, &bit);
	if (ret < 0) {
		return ENOMEM;
	}

	*lock = mark_pthread_obj_initialized(bit);

	return 0;
}

int pthread_spin_destroy(pthread_spinlock_t *lock)
{
	int err;
	size_t bit;
	struct k_spinlock *l;

	l = get_posix_spinlock(lock);
	if (l == NULL) {
		/* not specified as part of POSIX but this is the Linux behavior */
		return EINVAL;
	}

	bit = posix_spinlock_to_offset(l);
	err = sys_bitarray_free(&posix_spinlock_bitarray, 1, bit);
	__ASSERT_NO_MSG(err == 0);

	return 0;
}

int pthread_spin_lock(pthread_spinlock_t *lock)
{
	size_t bit;
	struct k_spinlock *l;

	l = get_posix_spinlock(lock);
	if (l == NULL) {
		/* not specified as part of POSIX but this is the Linux behavior */
		return EINVAL;
	}

	bit = posix_spinlock_to_offset(l);
	posix_spinlock_key[bit] = k_spin_lock(l);

	return 0;
}

int pthread_spin_trylock(pthread_spinlock_t *lock)
{
	size_t bit;
	struct k_spinlock *l;

	l = get_posix_spinlock(lock);
	if (l == NULL) {
		/* not specified as part of POSIX but this is the Linux behavior */
		return EINVAL;
	}

	bit = posix_spinlock_to_offset(l);
	return k_spin_trylock(l, &posix_spinlock_key[bit]);
}

int pthread_spin_unlock(pthread_spinlock_t *lock)
{
	size_t bit;
	struct k_spinlock *l;

	l = get_posix_spinlock(lock);
	if (l == NULL) {
		/* not specified as part of POSIX but this is the Linux behavior */
		return EINVAL;
	}

	bit = posix_spinlock_to_offset(l);
	k_spin_unlock(l, posix_spinlock_key[bit]);

	return 0;
}
