/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc.h"

#ifndef CONFIG_MCUBOOT
extern int _rodata_reserved_start;
extern int _rodata_reserved_end;
extern void rom_config_data_cache_mode(uint32_t cfg_cache_size, uint8_t cfg_cache_ways,
						uint8_t cfg_cache_line_size);
extern void rom_config_instruction_cache_mode(uint32_t cfg_cache_size, uint8_t cfg_cache_ways,
						uint8_t cfg_cache_line_size);
extern uint32_t Cache_Set_IDROM_MMU_Size(uint32_t irom_size, uint32_t drom_size);
extern void Cache_Set_IDROM_MMU_Info(uint32_t instr_page_num, uint32_t rodata_page_num,
					uint32_t rodata_start, uint32_t rodata_end,
					int i_off, int ro_off);
extern void Cache_Enable_ICache(uint32_t autoload);

void IRAM_ATTR esp_config_instruction_cache_mode(void)
{
	rom_config_instruction_cache_mode(CONFIG_ESP32S3_INSTRUCTION_CACHE_SIZE,
					CONFIG_ESP32S3_ICACHE_ASSOCIATED_WAYS,
					CONFIG_ESP32S3_INSTRUCTION_CACHE_LINE_SIZE);

	Cache_Suspend_DCache();
}

void IRAM_ATTR esp_config_data_cache_mode(void)
{
	int s_instr_flash2spiram_off = 0;
	int s_rodata_flash2spiram_off = 0;

	rom_config_data_cache_mode(CONFIG_ESP32S3_DATA_CACHE_SIZE,
				CONFIG_ESP32S3_DCACHE_ASSOCIATED_WAYS,
				CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE);
	Cache_Resume_DCache(0);

	/* Configure the Cache MMU size for instruction and rodata in flash. */
	uint32_t rodata_reserved_start_align =
		(uint32_t)&_rodata_reserved_start & ~(MMU_PAGE_SIZE - 1);
	uint32_t cache_mmu_irom_size =
		((rodata_reserved_start_align - SOC_DROM_LOW) / MMU_PAGE_SIZE) * sizeof(uint32_t);
	uint32_t cache_mmu_drom_size =
		(((uint32_t)&_rodata_reserved_end - rodata_reserved_start_align + MMU_PAGE_SIZE - 1)
		/ MMU_PAGE_SIZE) * sizeof(uint32_t);
	Cache_Set_IDROM_MMU_Size(cache_mmu_irom_size, CACHE_DROM_MMU_MAX_END - cache_mmu_irom_size);

	Cache_Set_IDROM_MMU_Info(cache_mmu_irom_size / sizeof(uint32_t),
				cache_mmu_drom_size / sizeof(uint32_t),
				(uint32_t)&_rodata_reserved_start,
				(uint32_t)&_rodata_reserved_end,
				s_instr_flash2spiram_off,
				s_rodata_flash2spiram_off);
}
#endif /* CONFIG_MCUBOOT */
