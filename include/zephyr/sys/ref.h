/*
 * Copyright (c) 2023 Michael Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_REF_H_
#define ZEPHYR_INCLUDE_SYS_REF_H_

#include <limits.h>
#include <stdbool.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup sys_ref_apis Reference count APIs
 * @{
 */

/* atomics are usually `long`. If they are larger, that's fine because
 * we'll just artificially limit them . If they were smaller, we'd be unable
 * to check if an overflow happened. This wouldn't be necessary if the atomic
 * API told us the max value of an atomic.
 */
BUILD_ASSERT(sizeof(atomic_val_t) >= sizeof(long));

/**
 * @brief An atomic reference count.
 *
 * The main use case is to share the same data between several users and free
 * it when all references were released. This can also be used on SMP systems.
 */
struct sys_ref {
	atomic_t refs;
};

/** @brief Initialize sys_ref at compile-time. */
#define SYS_REF_INIT()                                                                             \
	{                                                                                          \
		.refs = ATOMIC_INIT(1),                                                            \
	}

/** @brief Initialize sys_ref at runtime. */
static inline void sys_ref_init(struct sys_ref *const ref)
{
	*ref = (struct sys_ref){
		.refs = ATOMIC_INIT(1),
	};
}

/**
 * @brief Create a new reference to `ref`.
 *
 * @return The new reference.
 */
static inline struct sys_ref *sys_ref(struct sys_ref *const ref)
{
	const atomic_val_t old_rc = atomic_inc(&ref->refs);

	__ASSERT_NO_MSG(old_rc <= LONG_MAX);

	/* Prevent warnings with assertions disabled. */
	(void)(old_rc);

	return ref;
}

/**
 * @brief Release the reference `ref`.
 *
 * @return true if `ref` was the last reference.
 *         false otherwise.
 */
static inline bool sys_unref(struct sys_ref *const ref)
{
	const atomic_val_t old_rc = atomic_dec(&ref->refs);

	__ASSERT_NO_MSG(old_rc > 0);

	return old_rc == 1;
}

/**
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_SYS_REF_H_ */
