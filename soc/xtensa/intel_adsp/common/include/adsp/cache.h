/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __COMMON_ADSP_CACHE_H__
#define __COMMON_ADSP_CACHE_H__

#include <xtensa/hal.h>

/* macros for data cache operations */
#define SOC_DCACHE_FLUSH(addr, size)		\
	xthal_dcache_region_writeback((addr), (size))
#define SOC_DCACHE_INVALIDATE(addr, size)	\
	xthal_dcache_region_invalidate((addr), (size))

#endif
