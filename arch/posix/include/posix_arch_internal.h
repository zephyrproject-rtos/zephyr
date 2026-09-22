/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_POSIX_INCLUDE_POSIX_ARCH_INTERNAL_H_
#define ZEPHYR_ARCH_POSIX_INCLUDE_POSIX_ARCH_INTERNAL_H_

#include <zephyr/toolchain.h>

#define PC_SAFE_CALL(a) pc_safe_call(a, #a)

#ifdef __cplusplus
extern "C" {
#endif

static inline void pc_safe_call(int test, const char *test_str)
{
	/* LCOV_EXCL_START */ /* See Note1 */
	if (unlikely(test)) {
		posix_print_error_and_exit("POSIX arch: Error on: %s\n",
					   test_str);
	}
	/* LCOV_EXCL_STOP */
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_POSIX_INCLUDE_POSIX_ARCH_INTERNAL_H_ */

/*
 * Note 1:
 *
 * All checks for the host pthreads functions which are wrapped by PC_SAFE_CALL
 * are meant to never fail, and therefore will not be covered.
 */
