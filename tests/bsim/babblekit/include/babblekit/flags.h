/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * Provides a way to set/clear and block on binary flags.
 *
 * These flags are often used to wait until the test has gotten in a particular
 * state, e.g. a connection is established or a message has been successfully
 * sent.
 *
 * These macros can only be called from Zephyr threads. They can't be called
 * from e.g. a bs_tests callback.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/atomic.h>

/* Declare a flag that has been defined in another file */
#define DECLARE_FLAG(flag) extern atomic_t flag

/* Define a new binary flag.
 * Use @ref DEFINE_FLAG_STATIC if defining flags with the same name in multiple files.
 *
 * @param flag Name of the flag
 */
#define DEFINE_FLAG(flag)  atomic_t flag = (atomic_t) false

/* Define a new, static binary flag.
 *
 * @param flag Name of the flag
 */
#define DEFINE_FLAG_STATIC(flag)  static atomic_t flag = (atomic_t) false

#define SET_FLAG(flag)     (void)atomic_set(&flag, (atomic_t) true)
#define UNSET_FLAG(flag)   (void)atomic_set(&flag, (atomic_t) false)

#define IS_FLAG_SET(flag) (atomic_get(&flag) != false)

/* Block until `flag` is equal to `val` */
#define WAIT_FOR_VAL(var, val)                                                                     \
	while (atomic_get(&var) != val) {                                                          \
		(void)k_sleep(K_MSEC(1));                                                          \
	}

/* Block until `flag` is true */
#define WAIT_FOR_FLAG(flag)                                                                        \
	while (!(bool)atomic_get(&flag)) {                                                         \
		(void)k_sleep(K_MSEC(1));                                                          \
	}

/* Block until `flag` is false */
#define WAIT_FOR_FLAG_UNSET(flag)                                                                  \
	while ((bool)atomic_get(&flag)) {                                                          \
		(void)k_sleep(K_MSEC(1));                                                          \
	}

/* Block until `flag` is true and set it to false */
#define TAKE_FLAG(flag)                                                                            \
	while (!(bool)atomic_cas(&flag, true, false)) {                                            \
		(void)k_sleep(K_MSEC(1));                                                          \
	}
