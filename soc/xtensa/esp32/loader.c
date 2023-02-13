/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/storage/flash_map.h>
#include <esp_log.h>

#include <esp32/rom/cache.h>
#include <soc/dport_reg.h>
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
	size_t _partition_offset = FIXED_PARTITION_OFFSET(slot0_partition);
	uint32_t _app_irom_start = _partition_offset +
		(uint32_t)&_image_irom_start;
	uint32_t _app_irom_size = (uint32_t)&_image_irom_size;
	uint32_t _app_irom_vaddr = (uint32_t)&_image_irom_vaddr;

	uint32_t _app_drom_start = _partition_offset +
		(uint32_t)&_image_drom_start;
	uint32_t _app_drom_size = (uint32_t)&_image_drom_size;
	uint32_t _app_drom_vaddr = (uint32_t)&_image_drom_vaddr;

	Cache_Read_Disable(0);
	Cache_Flush(0);
	/* Clear the MMU entries that are already set up,
	 * so the new app only has the mappings it creates.
	 */
	for (int i = 0; i < DPORT_FLASH_MMU_TABLE_SIZE; i++) {
		DPORT_PRO_FLASH_MMU_TABLE[i] =
			DPORT_FLASH_MMU_TABLE_INVALID_VAL;
	}

	uint32_t drom_vaddr_addr_aligned = _app_drom_vaddr & MMU_FLASH_MASK;
	uint32_t drom_page_count = bootloader_cache_pages_to_map(_app_drom_size,
			_app_drom_vaddr);
	rc = cache_flash_mmu_set(0, 0, drom_vaddr_addr_aligned, _app_drom_start
			& MMU_FLASH_MASK, 64, drom_page_count);
	rc |= cache_flash_mmu_set(1, 0, drom_vaddr_addr_aligned, _app_drom_start
			& MMU_FLASH_MASK, 64, drom_page_count);

	uint32_t irom_vaddr_addr_aligned = _app_irom_vaddr & MMU_FLASH_MASK;
	uint32_t irom_page_count = bootloader_cache_pages_to_map(_app_irom_size,
			_app_irom_vaddr);
	rc |= cache_flash_mmu_set(0, 0, irom_vaddr_addr_aligned, _app_irom_start
			& MMU_FLASH_MASK, 64, irom_page_count);
	rc |= cache_flash_mmu_set(1, 0, irom_vaddr_addr_aligned, _app_irom_start
			& MMU_FLASH_MASK, 64, irom_page_count);

	DPORT_REG_CLR_BIT(DPORT_PRO_CACHE_CTRL1_REG,
			(DPORT_PRO_CACHE_MASK_IRAM0) |
			(DPORT_PRO_CACHE_MASK_IRAM1 & 0) |
			(DPORT_PRO_CACHE_MASK_IROM0 & 0) |
			DPORT_PRO_CACHE_MASK_DROM0 |
			DPORT_PRO_CACHE_MASK_DRAM1);

	DPORT_REG_CLR_BIT(DPORT_APP_CACHE_CTRL1_REG,
			(DPORT_APP_CACHE_MASK_IRAM0) |
			(DPORT_APP_CACHE_MASK_IRAM1 & 0) |
			(DPORT_APP_CACHE_MASK_IROM0 & 0) |
			DPORT_APP_CACHE_MASK_DROM0 |
			DPORT_APP_CACHE_MASK_DRAM1);

	esp_rom_Cache_Read_Enable(0);
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
