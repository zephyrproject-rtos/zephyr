/*
 * Copyright (c) 2023 Michael Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_KREF_H_
#define ZEPHYR_INCLUDE_SYS_KREF_H_

#include <stdbool.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup kref_apis Reference count APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief An atomic reference count.
 *
 * The main use case is to share the same data between several users and free
 * it when the count drops to 0. This can also be used on SMP systems.
 */
struct k_ref {
	atomic_t refs;
};

/** @brief Initialize k_ref at compile-time. */
#define K_REF_INIT()                                                                               \
	{                                                                                          \
		.refs = ATOMIC_INIT(1),                                                            \
	}

/** @brief Initialize k_ref at runtime. */
void k_ref_init(struct k_ref *ref);

/**
 * @brief Increase the reference count.
 *
 * @return `ref` is successful or
 *         NULL if the reference count was 0 already.
 */
struct k_ref *k_ref_get(struct k_ref *ref);

/**
 * @brief Decrease the reference count.
 *
 * @return true if the count reached 0 or
 *         false if there are still other references.
 */
bool k_ref_put(struct k_ref *ref);

/**
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_SYS_KREF_H_ */
