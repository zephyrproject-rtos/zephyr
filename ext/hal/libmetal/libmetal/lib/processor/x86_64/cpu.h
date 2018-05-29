/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	cpu.h
 * @brief	CPU specific primatives
 */

#ifndef __METAL_X86_64_CPU__H__
#define __METAL_X86_64_CPU__H__

#define metal_cpu_yield() asm volatile("rep; nop")

#endif /* __METAL_X86_64_CPU__H__ */
