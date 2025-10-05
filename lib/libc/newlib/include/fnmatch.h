/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_FNMATCH_H_
#define ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_FNMATCH_H_

#include_next <fnmatch.h>

/* Note: this is a GNU Extension */
#ifndef FNM_LEADING_DIR
#define FNM_LEADING_DIR 0x08
#endif

#endif /* ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_FNMATCH_H_ */
