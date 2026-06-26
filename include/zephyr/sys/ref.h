/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_REF_H_
#define ZEPHYR_INCLUDE_SYS_REF_H_

#include <limits.h>
#include <stdbool.h>

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reference counting
 * @defgroup sys_ref Reference counting
 * @ingroup utilities
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 * Value the counter is pinned to once it would overflow. A counter that
 * reaches this value can no longer be incremented or decremented, so the
 * referenced object is leaked instead of being freed too early.
 */
#define SYS_REFCOUNT_SATURATED	LONG_MAX
/** @endcond */

/**
 * @brief Reference counter
 *
 * Saturating reference counter. This is the minimal primitive used to
 * implement @ref sys_ref and is not meant to be used on its own by most
 * code. Unlike a plain @ref atomic_t, it is pinned on overflow and underflow
 * is caught by an assertion, turning a counting bug into a leak rather than a
 * use-after-free.
 */
struct sys_refcount {
	atomic_t value;
};

/**
 * @brief Static initializer for a @ref sys_refcount.
 *
 * Initializes the counter to one at compile time, equivalent to calling
 * sys_refcount_init().
 */
#define SYS_REFCOUNT_INITIALIZER { .value = ATOMIC_INIT(1) }

/**
 * @brief Initialize a reference counter to one.
 *
 * @param[in] rc Reference counter
 */
static inline void sys_refcount_init(struct sys_refcount *const rc)
{
	atomic_set(&rc->value, 1);
}

/**
 * @brief Increment a reference counter.
 *
 * The caller must already hold a reference, hence the counter must not be
 * zero. Once the counter is saturated it stays pinned.
 *
 * @param[in] rc Reference counter
 */
static inline void sys_refcount_inc(struct sys_refcount *const rc)
{
	atomic_val_t old = atomic_get(&rc->value);

	while (true) {
		__ASSERT(old != 0, "Increment of zero reference counter");

		if (old == SYS_REFCOUNT_SATURATED) {
			return;
		}

		if (atomic_cas(&rc->value, old, old + 1)) {
			return;
		}

		old = atomic_get(&rc->value);
	}
}

/**
 * @brief Decrement a reference counter.
 *
 * The counter must not be zero. A saturated counter stays pinned and never
 * reaches zero.
 *
 * @param[in] rc Reference counter
 *
 * @retval true if the counter reached zero
 * @retval false otherwise
 */
static inline bool sys_refcount_dec(struct sys_refcount *const rc)
{
	atomic_val_t old = atomic_get(&rc->value);

	while (true) {
		if (old == SYS_REFCOUNT_SATURATED) {
			return false;
		}

		__ASSERT(old > 0, "Decrement of zero reference counter");

		if (atomic_cas(&rc->value, old, old - 1)) {
			return old == 1;
		}

		old = atomic_get(&rc->value);
	}
}

struct sys_ref;

/**
 * @brief Release handler called once the last reference is dropped.
 *
 * @param[in] ref Reference embedded in the object being released
 */
typedef void (*sys_ref_release_t)(struct sys_ref *const ref);

/**
 * @brief Reference of an object
 *
 * Embed this in an object whose lifetime is shared between several users. The
 * object is released through the handler passed to sys_ref_put() once the last
 * reference is dropped.
 */
struct sys_ref {
	struct sys_refcount refcount;
};

/**
 * @brief Static initializer for a @ref sys_ref.
 *
 * Establishes the first reference at compile time, equivalent to calling
 * sys_ref_init():
 *
 * @code{.c}
 * struct my_obj obj = {
 *         .ref = SYS_REF_INITIALIZER,
 *         ...
 * };
 * @endcode
 */
#define SYS_REF_INITIALIZER { .refcount = SYS_REFCOUNT_INITIALIZER }

/**
 * @brief Initialize a reference to one.
 *
 * Must be called once before any other operation on the reference, by whoever
 * creates the object. It establishes the first reference, owned by the caller
 * and released with sys_ref_put().
 *
 * @param[in] ref Reference
 */
static inline void sys_ref_init(struct sys_ref *const ref)
{
	sys_refcount_init(&ref->refcount);
}

/**
 * @brief Increase reference of an object.
 *
 * Only safe to call while the caller already holds a valid reference, which
 * keeps the object alive and the counter non-zero. It cannot be used to take
 * the first reference (see sys_ref_init()) or to revive an object whose
 * references may have already dropped to zero.
 *
 * @param[in] ref Reference
 */
static inline void sys_ref_get(struct sys_ref *const ref)
{
	sys_refcount_inc(&ref->refcount);
}

/**
 * @brief Decrease reference of an object.
 *
 * Decrement the reference and call @p release once the last one is dropped.
 * After this call the caller must no longer access the object, as it may
 * already have been released.
 *
 * The release handler recovers the enclosing object with @ref CONTAINER_OF and
 * frees the resources it holds:
 *
 * @code{.c}
 * struct my_obj {
 *         struct sys_ref ref;
 *         ...
 * };
 *
 * static void my_obj_release(struct sys_ref *const ref)
 * {
 *         struct my_obj *obj = CONTAINER_OF(ref, struct my_obj, ref);
 *
 *         k_free(obj);
 * }
 *
 * sys_ref_put(&obj->ref, my_obj_release);
 * @endcode
 *
 * @param[in] ref Reference
 * @param[in] release Handler releasing the object
 *
 * @retval true if the object was released
 * @retval false otherwise
 */
static inline bool sys_ref_put(struct sys_ref *const ref,
			       const sys_ref_release_t release)
{
	__ASSERT_NO_MSG(release != NULL);

	if (sys_refcount_dec(&ref->refcount)) {
		release(ref);
		return true;
	}

	return false;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_REF_H_ */
