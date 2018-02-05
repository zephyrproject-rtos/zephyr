/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _POSIX_INTERNAL_H
#define _POSIX_INTERNAL_H

#define _SAFE_CALL(a) /* See Note 1 below */ \
	do { \
		if (a) { \
			posix_print_error_and_exit(ERPREFIX #a "\n"); \
		} \
	} while (0)

#ifdef __cplusplus
extern "C" {
#endif


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
