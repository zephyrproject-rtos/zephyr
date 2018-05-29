/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	cpu.h
 * @brief	CPU specific primatives
 */

#ifndef __METAL_AARCH64_CPU__H__
#define __METAL_AARCH64_CPU__H__

#define metal_cpu_yield() asm volatile("yield")

#endif /* __METAL_AARCH64_CPU__H__ */
