/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __COMMON_ADSP_CACHE_H__
#define __COMMON_ADSP_CACHE_H__

#include <xtensa/hal.h>

/* Macros for data cache operations */
#define SOC_DCACHE_FLUSH(addr, size)		\
	sys_cache_data_flush_range((addr), (size))
#define SOC_DCACHE_INVALIDATE(addr, size)	\
	sys_cache_data_invd_range((addr), (size))

#endif
