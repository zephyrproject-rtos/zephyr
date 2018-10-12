/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	compiler.h
 * @brief	Compiler specific primitives for libmetal.
 */

#ifndef __METAL_COMPILER__H__
#define __METAL_COMPILER__H__

#if defined(__GNUC__)
# include <metal/compiler/gcc/compiler.h>
#elif defined(__ICCARM__)
# include <metal/compiler/iar/compiler.h>
#else
# error "Missing compiler support"
#endif

#endif /* __METAL_COMPILER__H__ */
