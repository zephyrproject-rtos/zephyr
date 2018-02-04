/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _POSIX_INTERNAL_H
#define _POSIX_INTERNAL_H

#define _SAFE_CALL(a) _safe_call(a, #a)

#ifdef __cplusplus
extern "C" {
#endif

static inline void _safe_call(int test, const char *test_str)
{
	/* LCOV_EXCL_START */ /* See Note1 */
	if (test) {
		posix_print_error_and_exit("POSIX arch: Error on: %s\n",
					   test_str);
	}
	/* LCOV_EXCL_STOP */
}

#ifdef __cplusplus
}
#endif

#endif /* _POSIX_INTERNAL_H */

/*
 * Note 1:
 *
 * All checks for the host pthreads functions which are wrapped by _SAFE_CALL
 * are meant to never fail, and therefore will not be covered.
 */
