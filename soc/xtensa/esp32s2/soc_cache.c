/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc.h"

/*
 * Instruction Cache definitions
 */
#if defined(CONFIG_ESP32S2_INSTRUCTION_CACHE_8KB)
#define ESP32S2_ICACHE_SIZE CACHE_SIZE_8KB
#else
#define ESP32S2_ICACHE_SIZE CACHE_SIZE_16KB
#endif

#if defined(CONFIG_ESP32S2_INSTRUCTION_CACHE_LINE_16B)
#define ESP32S2_ICACHE_LINE_SIZE CACHE_LINE_SIZE_16B
#else
#define ESP32S2_ICACHE_LINE_SIZE CACHE_LINE_SIZE_32B
#endif

/*
 * Data Cache definitions
 */
#if defined(CONFIG_ESP32S2_DATA_CACHE_8KB)
#define ESP32S2_DCACHE_SIZE CACHE_SIZE_8KB
#else
#define ESP32S2_DCACHE_SIZE CACHE_SIZE_16KB
#endif

#if defined(CONFIG_ESP32S2_DATA_CACHE_LINE_16B)
#define ESP32S2_DCACHE_LINE_SIZE CACHE_LINE_SIZE_16B
#else
#define ESP32S2_DCACHE_LINE_SIZE CACHE_LINE_SIZE_32B
#endif

void IRAM_ATTR esp_config_instruction_cache_mode(void)
{
	cache_size_t cache_size;
	cache_ways_t cache_ways;
	cache_line_size_t cache_line_size;

#if CONFIG_ESP32S2_INSTRUCTION_CACHE_8KB
	esp_rom_Cache_Allocate_SRAM(CACHE_MEMORY_ICACHE_LOW, CACHE_MEMORY_INVALID,
				CACHE_MEMORY_INVALID, CACHE_MEMORY_INVALID);
#else
	esp_rom_Cache_Allocate_SRAM(CACHE_MEMORY_ICACHE_LOW, CACHE_MEMORY_ICACHE_HIGH,
				CACHE_MEMORY_INVALID, CACHE_MEMORY_INVALID);
#endif

	cache_size = ESP32S2_ICACHE_SIZE;
	cache_ways = CACHE_4WAYS_ASSOC;
	cache_line_size = ESP32S2_ICACHE_LINE_SIZE;

	esp_rom_Cache_Suspend_ICache();
	esp_rom_Cache_Set_ICache_Mode(cache_size, cache_ways, cache_line_size);
	esp_rom_Cache_Invalidate_ICache_All();
	esp_rom_Cache_Resume_ICache(0);
}

void IRAM_ATTR esp_config_data_cache_mode(void)
{
	cache_size_t cache_size;
	cache_ways_t cache_ways;
	cache_line_size_t cache_line_size;

#if CONFIG_ESP32S2_INSTRUCTION_CACHE_8KB
#if CONFIG_ESP32S2_DATA_CACHE_8KB
	esp_rom_Cache_Allocate_SRAM(CACHE_MEMORY_ICACHE_LOW, CACHE_MEMORY_DCACHE_LOW,
				CACHE_MEMORY_INVALID, CACHE_MEMORY_INVALID);
#else
	esp_rom_Cache_Allocate_SRAM(CACHE_MEMORY_ICACHE_LOW, CACHE_MEMORY_DCACHE_LOW,
				CACHE_MEMORY_DCACHE_HIGH, CACHE_MEMORY_INVALID);
#endif
#else
#if CONFIG_ESP32S2_DATA_CACHE_8KB
	esp_rom_Cache_Allocate_SRAM(CACHE_MEMORY_ICACHE_LOW, CACHE_MEMORY_ICACHE_HIGH,
				CACHE_MEMORY_DCACHE_LOW, CACHE_MEMORY_INVALID);
#else
	esp_rom_Cache_Allocate_SRAM(CACHE_MEMORY_ICACHE_LOW, CACHE_MEMORY_ICACHE_HIGH,
				CACHE_MEMORY_DCACHE_LOW, CACHE_MEMORY_DCACHE_HIGH);
#endif
#endif
	cache_size = ESP32S2_DCACHE_SIZE;
	cache_ways = CACHE_4WAYS_ASSOC;
	cache_line_size = ESP32S2_DCACHE_LINE_SIZE;

	esp_rom_Cache_Set_DCache_Mode(cache_size, cache_ways, cache_line_size);
	esp_rom_Cache_Invalidate_DCache_All();
}
