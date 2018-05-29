/*
 * Copyright (c) 2017, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/io.h
 * @brief	Zephyr specific io definitions.
 */

#ifndef __METAL_IO__H__
#error "Include metal/io.h instead of metal/zephyr/io.h"
#endif

#ifndef __METAL_ZEPHYR_IO__H__
#define __METAL_ZEPHYR_IO__H__

#include <stdlib.h>
#include <metal/utilities.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef METAL_INTERNAL

/**
 * @brief memory mapping for an I/O region
 */
static inline void metal_sys_io_mem_map(struct metal_io_region *io)
{
	metal_unused(io);
}

/**
 * @brief memory mapping
 */
static inline void *metal_machine_io_mem_map(void *va, metal_phys_addr_t pa,
					     size_t size, unsigned int flags)
{
	metal_unused(pa);
	metal_unused(size);
	metal_unused(flags);

	return va;
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* __METAL_ZEPHYR_IO__H__ */
