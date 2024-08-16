/*
 * Copyright (c) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_COMMON_UTIL_H_
#define ZEPHYR_SOC_INTEL_ADSP_COMMON_UTIL_H_

#include <zephyr/cache.h>

/* memcopy used by boot loader */
static ALWAYS_INLINE void bmemcpy(void *dest, void *src, size_t bytes)
{
	volatile uint32_t *d = (uint32_t *)dest;
	volatile uint32_t *s = (uint32_t *)src;

	sys_cache_data_invd_range(src, bytes);
	for (size_t i = 0; i < (bytes >> 2); i++) {
		d[i] = s[i];
	}

	sys_cache_data_flush_range(dest, bytes);
}

/* bzero used by bootloader */
static ALWAYS_INLINE void bbzero(void *dest, size_t bytes)
{
	volatile uint32_t *d = (uint32_t *)dest;

	for (size_t i = 0; i < (bytes >> 2); i++) {
		d[i] = 0;
	}

	sys_cache_data_flush_range(dest, bytes);
}

#endif /* ZEPHYR_SOC_INTEL_ADSP_COMMON_UTIL_H_ */
