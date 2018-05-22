/*
 * Copyright (c) 2018, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	assert.h
 * @brief	Zephyr assertion support.
 */

#ifndef __METAL_ASSERT__H__
#error "Include metal/assert.h instead of metal/zephyr/assert.h"
#endif

#ifndef __METAL_ZEPHYR_ASSERT__H__
#define __METAL_ZEPHYR_ASSERT__H__

#include <zephyr.h>

/**
 * @brief Assertion macro for Zephyr-based applications.
 * @param cond Condition to evaluate.
 */
#define metal_sys_assert(cond) __ASSERT_NO_MSG(cond)

#endif /* __METAL_ZEPHYR_ASSERT__H__ */

