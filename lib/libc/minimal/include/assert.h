/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_assert_h__
#define __INC_assert_h__

#include <misc/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NDEBUG
#ifndef assert
#define assert(test) __ASSERT_NO_MSG(test)
#endif
#else
#ifndef assert
#define assert(test)
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif  /* __INC_assert_h__ */
