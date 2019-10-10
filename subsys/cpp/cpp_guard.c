/*
 * Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/__assert.h>

static K_MUTEX_DEFINE(guard_mutex);

static bool init_has_run(uint64_t *guard_object)
{
	return (((uint8_t *)guard_object)[0] != 0);
}

static void set_init_has_run(uint64_t *guard_object)
{
	((uint8_t *)guard_object)[0] = 1;
}

/*
 * This function is called before initialization takes place. If this
 * function returns 1, either __cxa_guard_relase or __cxa_guard_abort
 * must be called with the same argument. The first byte of the guard_object
 * is not modified by this function.
 *
 * Returns 1 if the initialization in not yet complete, otherwise 0.
 */
int __cxa_guard_acquire(uint64_t *guard_object)
{
	int res;

	if (init_has_run(guard_object)) {
		return 0;
	}

	res = k_mutex_lock(&guard_mutex, K_FOREVER);
	__ASSERT_NO_MSG(res == 0);

	if (init_has_run(guard_object)) {
		k_mutex_unlock(&guard_mutex);

		return 0;
	}

	return 1;
}

/*
 * This function is called after initialization is complete. Sets the first
 * byte of the guard object to a non-zero value.
 */
void __cxa_guard_release(uint64_t *guard_object)
{
	set_init_has_run(guard_object);

	k_mutex_unlock(&guard_mutex);
}

/*
 * This function is called if the initialization terminates by throwing an
 * exception.
 */
void __cxa_guard_abort(uint64_t *guard_object)
{
	k_mutex_unlock(&guard_mutex);
}
