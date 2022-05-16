/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/zephyr.h>
#include <soc.h>
#include <zephyr/storage/flash_map.h>
#include <esp_log.h>
#include <stdlib.h>

#include <esp32s2/rom/cache.h>
#include "soc/cache_memory.h"
#include "soc/extmem_reg.h"
#include <bootloader_flash_priv.h>

#ifdef CONFIG_BOOTLOADER_MCUBOOT
#define HDR_ATTR __attribute__((section(".entry_addr"))) __attribute__((used))

extern uint32_t _image_irom_start, _image_irom_size, _image_irom_vaddr;
extern uint32_t _image_drom_start, _image_drom_size, _image_drom_vaddr;

void __start(void);

static HDR_ATTR void (*_entry_point)(void) = &__start;

static int map_rom_segments(void)
{
	int rc = 0;

	size_t _partition_offset = FLASH_AREA_OFFSET(image_0);
	uint32_t _app_irom_start = _partition_offset + (uint32_t)&_image_irom_start;
	uint32_t _app_irom_size = (uint32_t)&_image_irom_size;
	uint32_t _app_irom_vaddr = (uint32_t)&_image_irom_vaddr;

	uint32_t _app_drom_start = _partition_offset + (uint32_t)&_image_drom_start;
	uint32_t _app_drom_size = (uint32_t)&_image_drom_size;
	uint32_t _app_drom_vaddr = (uint32_t)&_image_drom_vaddr;

	uint32_t autoload = esp_rom_Cache_Suspend_ICache();

	esp_rom_Cache_Invalidate_ICache_All();

	/* Clear the MMU entries that are already set up,
	 * so the new app only has the mappings it creates.
	 */
	for (size_t i = 0; i < FLASH_MMU_TABLE_SIZE; i++) {
		FLASH_MMU_TABLE[i] = MMU_TABLE_INVALID_VAL;
	}

	uint32_t drom_page_count = bootloader_cache_pages_to_map(_app_drom_size, _app_drom_vaddr);

	rc |= esp_rom_Cache_Ibus_MMU_Set(MMU_ACCESS_FLASH, _app_drom_vaddr & 0xffff0000,
				_app_drom_start & 0xffff0000, 64, drom_page_count, 0);

	uint32_t irom_page_count = bootloader_cache_pages_to_map(_app_irom_size, _app_irom_vaddr);

	if (_app_irom_vaddr + _app_irom_size > IRAM1_ADDRESS_LOW) {
		rc |= esp_rom_Cache_Ibus_MMU_Set(MMU_ACCESS_FLASH, IRAM0_ADDRESS_LOW, 0, 64, 64, 1);
		rc |= esp_rom_Cache_Ibus_MMU_Set(MMU_ACCESS_FLASH, IRAM1_ADDRESS_LOW, 0, 64, 64, 1);
		REG_CLR_BIT(EXTMEM_PRO_ICACHE_CTRL1_REG, EXTMEM_PRO_ICACHE_MASK_IRAM1);
	}

	rc |= esp_rom_Cache_Ibus_MMU_Set(MMU_ACCESS_FLASH, _app_irom_vaddr & 0xffff0000,
				_app_irom_start & 0xffff0000, 64, irom_page_count, 0);

	REG_CLR_BIT(EXTMEM_PRO_ICACHE_CTRL1_REG, (EXTMEM_PRO_ICACHE_MASK_IRAM0) |
				(EXTMEM_PRO_ICACHE_MASK_IRAM1 & 0) | EXTMEM_PRO_ICACHE_MASK_DROM0);

	esp_rom_Cache_Resume_ICache(autoload);

	return rc;
}
#endif

void __start(void)
{
#ifdef CONFIG_BOOTLOADER_MCUBOOT
	int err = map_rom_segments();

	if (err != 0) {
		ets_printf("Failed to setup XIP, aborting\n");
		abort();
	}
#endif
	__esp_platform_start();
}
