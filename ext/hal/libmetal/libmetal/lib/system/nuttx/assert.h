/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	assert.h
 * @brief	NuttX assertion support.
 */

#ifndef __METAL_ASSERT__H__
#error "Include metal/assert.h instead of metal/nuttx/assert.h"
#endif

#ifndef __METAL_NUTTX_ASSERT__H__
#define __METAL_NUTTX_ASSERT__H__

#include <assert.h>

/**
 * @brief Assertion macro for NuttX-based applications.
 * @param cond Condition to evaluate.
 */
#define metal_sys_assert(cond) DEBUGASSERT(cond)

#endif /* __METAL_NUTTX_ASSERT__H__ */
