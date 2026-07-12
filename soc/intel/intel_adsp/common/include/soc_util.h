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
	sys_cache_data_invd_range(src, bytes);

	if ((((uintptr_t)dest | (uintptr_t)src) & 3) == 0) {
		uint32_t *d32 = (uint32_t *)dest;
		uint32_t *s32 = (uint32_t *)src;
		size_t words = bytes / 4;
		size_t remainder = bytes % 4;

		for (size_t i = 0; i < words; i++) {
			d32[i] = s32[i];
		}

		uint8_t *d8 = (uint8_t *)(d32 + words);
		uint8_t *s8 = (uint8_t *)(s32 + words);
		for (size_t i = 0; i < remainder; i++) {
			d8[i] = s8[i];
		}
	} else {
		uint8_t *d8 = (uint8_t *)dest;
		uint8_t *s8 = (uint8_t *)src;
		for (size_t i = 0; i < bytes; i++) {
			d8[i] = s8[i];
		}
	}

	sys_cache_data_flush_range(dest, bytes);
}

static ALWAYS_INLINE void bbzero(void *dest, size_t bytes)
{
	uint32_t *d32 = (uint32_t *)dest;
	size_t words = bytes / 4;
	size_t remainder = bytes % 4;

	for (size_t i = 0; i < words; i++) {
		d32[i] = 0;
	}

	uint8_t *d8 = (uint8_t *)(d32 + words);
	for (size_t i = 0; i < remainder; i++) {
		d8[i] = 0;
	}

	sys_cache_data_flush_range(dest, bytes);
}

#endif /* ZEPHYR_SOC_INTEL_ADSP_COMMON_UTIL_H_ */
