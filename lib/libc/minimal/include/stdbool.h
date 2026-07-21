/* stdbool.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDBOOL_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDBOOL_H_

#if !defined(__cplusplus) && __STDC_VERSION__ < 202311L
#define bool   _Bool
#define true   1
#define false  0
#endif

#define __bool_true_false_are_defined  1

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDBOOL_H_ */
