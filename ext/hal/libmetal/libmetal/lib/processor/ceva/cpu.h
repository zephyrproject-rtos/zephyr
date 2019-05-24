/*
 * Copyright (c) 2017, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	cpu.h
 * @brief	CPU specific primitives
 */

#ifndef __METAL_CEVA_CPU__H__
#define __METAL_CEVA_CPU__H__

#define metal_cpu_yield()

#ifdef TEAKLITE4
/*
 * The dummy implementation is enough here since
 * tl42x don't support the out of order and multi core
 */
#define __sync_synchronize()
#endif

#endif /* __METAL_CEVA_CPU__H__ */
