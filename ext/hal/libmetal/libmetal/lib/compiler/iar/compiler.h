/*
 * Copyright (c) 2018, ST Microelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	iar/compiler.h
 * @brief	IAR specific primitives for libmetal.
 */

#ifndef __METAL_IAR_COMPILER__H__
#define __METAL_IAR_COMPILER__H__

#ifdef __cplusplus
extern "C" {
#endif

#define restrict __restrict__
#define metal_align(n) __attribute__((aligned(n)))
#define metal_weak __attribute__((weak))

#ifdef __cplusplus
}
#endif

#endif /* __METAL_IAR_COMPILER__H__ */
