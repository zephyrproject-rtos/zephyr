/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_INCL_NSI_UTILS_H
#define NSI_COMMON_SRC_INCL_NSI_UTILS_H

/* Remove brackets from around a single argument: */
#define NSI_DEBRACKET(...) __VA_ARGS__

#define _NSI_STRINGIFY(x) #x
#define NSI_STRINGIFY(s) _NSI_STRINGIFY(s)

/* concatenate the values of the arguments into one */
#define NSI_DO_CONCAT(x, y) x ## y
#define NSI_CONCAT(x, y) NSI_DO_CONCAT(x, y)

#define NSI_MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define NSI_MIN(a, b) (((a) < (b)) ? (a) : (b))

#ifndef NSI_ARG_UNUSED
#define NSI_ARG_UNUSED(x) (void)(x)
#endif

#endif /* NSI_COMMON_SRC_INCL_NSI_UTILS_H */
