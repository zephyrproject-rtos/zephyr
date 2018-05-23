/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	gcc/compiler.h
 * @brief	GCC specific primitives for libmetal.
 */

#ifndef __METAL_GCC_COMPILER__H__
#define __METAL_GCC_COMPILER__H__

#ifdef __cplusplus
extern "C" {
#endif

#define restrict __restrict__
#define metal_align(n) __attribute__((aligned(n)))
#define metal_weak __attribute__((weak))

#ifdef __cplusplus
}
#endif

#endif /* __METAL_GCC_COMPILER__H__ */
