/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc.h"

/*
 * Instruction Cache definitions
 */
#if defined(CONFIG_ESP32S3_INSTRUCTION_CACHE_16KB)
#define ESP32S3_ICACHE_SIZE ICACHE_SIZE_16KB
#else
#define ESP32S3_ICACHE_SIZE ICACHE_SIZE_32KB
#endif

#if defined(CONFIG_ESP32S3_INSTRUCTION_CACHE_LINE_16B)
#define ESP32S3_ICACHE_LINE_SIZE CACHE_LINE_SIZE_16B
#else
#define ESP32S3_ICACHE_LINE_SIZE CACHE_LINE_SIZE_32B
#endif

/*
 * Data Cache definitions
 */
#if defined(CONFIG_ESP32S3_DATA_CACHE_16KB) || defined(CONFIG_ESP32S3_DATA_CACHE_32KB)
#define ESP32S3_DCACHE_SIZE DCACHE_SIZE_32KB
#else
#define ESP32S3_DCACHE_SIZE DCACHE_SIZE_64KB
#endif

#if defined(CONFIG_ESP32S3_DATA_CACHE_LINE_16B)
#define ESP32S3_DCACHE_LINE_SIZE CACHE_LINE_SIZE_16B
#elif defined(CONFIG_ESP32S3_DATA_CACHE_LINE_32B)
#define ESP32S3_DCACHE_LINE_SIZE CACHE_LINE_SIZE_32B
#else
#define ESP32S3_DCACHE_LINE_SIZE CACHE_LINE_SIZE_32B
#endif

#define CACHE_MEMORY_ICACHE_LOW CACHE_ICACHE0

#ifndef CONFIG_MCUBOOT
extern void Cache_Enable_ICache(uint32_t autoload);

void IRAM_ATTR esp_config_instruction_cache_mode(void)
{
	cache_size_t cache_size;
	cache_ways_t cache_ways;
	cache_line_size_t cache_line_size;

#if CONFIG_ESP32S3_INSTRUCTION_CACHE_16KB
	Cache_Occupy_ICache_MEMORY(CACHE_MEMORY_IBANK0, CACHE_MEMORY_INVALID);
	cache_size = CACHE_SIZE_HALF;
#else
	Cache_Occupy_ICache_MEMORY(CACHE_MEMORY_IBANK0, CACHE_MEMORY_IBANK1);
	cache_size = CACHE_SIZE_FULL;
#endif
#if CONFIG_ESP32S3_INSTRUCTION_CACHE_4WAYS
	cache_ways = CACHE_4WAYS_ASSOC;
#else
	cache_ways = CACHE_8WAYS_ASSOC;
#endif
#if CONFIG_ESP32S3_INSTRUCTION_CACHE_LINE_16B
	cache_line_size = CACHE_LINE_SIZE_16B;
#elif CONFIG_ESP32S3_INSTRUCTION_CACHE_LINE_32B
	cache_line_size = CACHE_LINE_SIZE_32B;
#else
	cache_line_size = CACHE_LINE_SIZE_64B;
#endif
	Cache_Set_ICache_Mode(cache_size, cache_ways, cache_line_size);
	Cache_Invalidate_ICache_All();
	Cache_Enable_ICache(0);
}

void IRAM_ATTR esp_config_data_cache_mode(void)
{
	cache_size_t cache_size;
	cache_ways_t cache_ways;
	cache_line_size_t cache_line_size;

#if CONFIG_ESP32S3_DATA_CACHE_32KB
	Cache_Occupy_DCache_MEMORY(CACHE_MEMORY_DBANK1, CACHE_MEMORY_INVALID);
	cache_size = CACHE_SIZE_HALF;
#else
	Cache_Occupy_DCache_MEMORY(CACHE_MEMORY_DBANK0, CACHE_MEMORY_DBANK1);
	cache_size = CACHE_SIZE_FULL;
#endif
#if CONFIG_ESP32S3_DATA_CACHE_4WAYS
	cache_ways = CACHE_4WAYS_ASSOC;
#else
	cache_ways = CACHE_8WAYS_ASSOC;
#endif
#if CONFIG_ESP32S3_DATA_CACHE_LINE_16B
	cache_line_size = CACHE_LINE_SIZE_16B;
#elif CONFIG_ESP32S3_DATA_CACHE_LINE_32B
	cache_line_size = CACHE_LINE_SIZE_32B;
#else
	cache_line_size = CACHE_LINE_SIZE_64B;
#endif
	Cache_Set_DCache_Mode(cache_size, cache_ways, cache_line_size);
	Cache_Invalidate_DCache_All();
}
#endif /* CONFIG_MCUBOOT */
