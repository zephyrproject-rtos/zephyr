/*
 * Copyright (c) 2017, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	generic/microblaze_generic/sys.h
 * @brief	generic microblaze system primitives for libmetal.
 */

#ifndef __METAL_GENERIC_SYS__H__
#error "Include metal/sys.h instead of metal/generic/@PROJECT_MACHINE@/sys.h"
#endif

#ifndef __METAL_GENERIC_MICROBLAZE_SYS__H__
#define __METAL_GENERIC_MICROBLAZE_SYS__H__

#include <metal/compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef METAL_INTERNAL

void metal_weak sys_irq_enable(unsigned int vector);

void metal_weak sys_irq_disable(unsigned int vector);

#endif /* METAL_INTERNAL */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_GENERIC_MICROBLAZE_SYS__H__ */
