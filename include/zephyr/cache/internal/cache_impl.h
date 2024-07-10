/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Inline syscall implementation for cache APIs
 */

#ifndef ZEPHYR_INCLUDE_CACHE_H_
#error "Should only be included by zephyr/cache.h"
#endif

#ifndef ZEPHYR_INCLUDE_CACHE_INTERNAL_CACHE_IMPL_H_
#define ZEPHYR_INCLUDE_CACHE_INTERNAL_CACHE_IMPL_H_

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/debug/sparse.h>

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE int z_impl_sys_cache_data_flush_range(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	return cache_data_flush_range(addr, size);
#endif
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

static ALWAYS_INLINE int z_impl_sys_cache_data_invd_range(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	return cache_data_invd_range(addr, size);
#endif
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

static ALWAYS_INLINE int z_impl_sys_cache_data_flush_and_invd_range(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	return cache_data_flush_and_invd_range(addr, size);
#endif
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CACHE_INTERNAL_CACHE_IMPL_H_ */
