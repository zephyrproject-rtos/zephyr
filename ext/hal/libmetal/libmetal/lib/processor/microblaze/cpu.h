/*
 * Copyright (c) 2017, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	cpu.h
 * @brief	CPU specific primatives on microblaze platform.
 */

#ifndef __METAL_MICROBLAZE__H__
#define __METAL_MICROBLAZE__H__

#include <stdint.h>
#include <metal/atomic.h>

#define metal_cpu_yield()

static inline void metal_processor_io_write64(void *ptr, uint64_t value,
					      memory_order order)
{
	void *tmp = &value;

	atomic_store_explicit((atomic_ulong *)ptr, *((atomic_ulong *)tmp), order);
	tmp += sizeof(atomic_ulong);
	ptr += sizeof(atomic_ulong);
	atomic_store_explicit((atomic_ulong *)ptr, *((atomic_ulong *)tmp), order);
}

static inline uint64_t metal_processor_io_read64(void *ptr, memory_order order)
{
	uint64_t long_ret;
	void *tmp = &long_ret;

	*((atomic_ulong *)tmp) = atomic_load_explicit((atomic_ulong *)ptr, order);
	tmp += sizeof(atomic_ulong);
	ptr += sizeof(atomic_ulong);
	*((atomic_ulong *)tmp) = atomic_load_explicit((atomic_ulong *)ptr, order);

	return long_ret;
}

#endif /* __METAL_MICROBLAZE__H__ */
