/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	nuttx/io.h
 * @brief	NuttX specific io definitions.
 */

#ifndef __METAL_IO__H__
#error "Include metal/io.h instead of metal/nuttx/io.h"
#endif

#ifndef __METAL_NUTTX_IO__H__
#define __METAL_NUTTX_IO__H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Get the default global io ops.
 * @return  an io ops.
 */
struct metal_io_ops *metal_io_get_ops(void);

/**
 * @brief  Get the default global io region.
 * @return  an io region.
 */
struct metal_io_region *metal_io_get_region(void);

#ifdef METAL_INTERNAL

/**
 * @brief memory mapping for an I/O region
 */
static inline void metal_sys_io_mem_map(struct metal_io_region *io)
{
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __METAL_NUTTX_IO__H__ */
