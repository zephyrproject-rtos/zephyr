/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_ASSERT_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_ASSERT_H_

#include <misc/__assert.h>

#ifdef __cplusplus
extern "C" {
#else
#define static_assert _Static_assert
#endif

#ifndef NDEBUG
#ifndef assert
#define assert(test) __ASSERT_NO_MSG(test)
#endif
#else
#ifndef assert
#define assert(test) ((void)0)
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_ASSERT_H_ */
