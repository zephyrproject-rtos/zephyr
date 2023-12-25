/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_POSIX_ZEPHYR_SOURCE_H_
#define ZEPHYR_LIB_POSIX_ZEPHYR_SOURCE_H_

/*
 * This header can be included for builds using the Minimal libc in order to make the required
 * POSIX application conformance options visible.
 */

#if defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || defined(_ZEPHYR_SOURCE) ||                    \
	(!defined(__STRICT_ANSI__) && !defined(_ANSI_SOURCE) && !defined(_ISOC99_SOURCE) &&        \
	 !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE))
#undef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif

#ifdef _DEFAULT_SOURCE
#undef _POSIX_SOURCE
#define _POSIX_SOURCE 1
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#endif /* ZEPHYR_LIB_POSIX_ZEPHYR_SOURCE_H_ */
