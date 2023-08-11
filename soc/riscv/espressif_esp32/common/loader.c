/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <hal/mmu_hal.h>
#include <hal/mmu_types.h>
#include <hal/cache_types.h>
#include <hal/cache_ll.h>
#include <hal/cache_hal.h>
#include <rom/cache.h>
#include <esp_rom_sys.h>

#define MMU_FLASH_MASK    (~(CONFIG_MMU_PAGE_SIZE - 1))

#ifdef CONFIG_BOOTLOADER_MCUBOOT
#include <zephyr/storage/flash_map.h>
#include "esp_rom_uart.h"

#define BOOT_LOG_INF(_fmt, ...) \
	ets_printf("[" CONFIG_SOC_SERIES "] [INF] " _fmt "\n\r", ##__VA_ARGS__)

#define HDR_ATTR __attribute__((section(".entry_addr"))) __attribute__((used))

void __start(void);

static HDR_ATTR void (*_entry_point)(void) = &__start;

extern uint32_t _image_irom_start, _image_irom_size, _image_irom_vaddr;
extern uint32_t _image_drom_start, _image_drom_size, _image_drom_vaddr;
#endif /* CONFIG_BOOTLOADER_MCUBOOT */

void map_rom_segments(uint32_t app_drom_start, uint32_t app_drom_vaddr,
				uint32_t app_drom_size, uint32_t app_irom_start,
				uint32_t app_irom_vaddr, uint32_t app_irom_size)
{
	uint32_t app_irom_start_aligned = app_irom_start & MMU_FLASH_MASK;
	uint32_t app_irom_vaddr_aligned = app_irom_vaddr & MMU_FLASH_MASK;

	uint32_t app_drom_start_aligned = app_drom_start & MMU_FLASH_MASK;
	uint32_t app_drom_vaddr_aligned = app_drom_vaddr & MMU_FLASH_MASK;
	uint32_t actual_mapped_len = 0;

	cache_hal_disable(CACHE_TYPE_ALL);

	/* Clear the MMU entries that are already set up,
	 * so the new app only has the mappings it creates.
	 */
	mmu_hal_unmap_all();

	mmu_hal_map_region(0, MMU_TARGET_FLASH0, app_drom_vaddr_aligned, app_drom_start_aligned,
				app_drom_size, &actual_mapped_len);

	mmu_hal_map_region(0, MMU_TARGET_FLASH0, app_irom_vaddr_aligned, app_irom_start_aligned,
				app_irom_size, &actual_mapped_len);

	/* ----------------------Enable corresponding buses---------------- */
	cache_bus_mask_t bus_mask = cache_ll_l1_get_bus(0, app_drom_vaddr_aligned, app_drom_size);

	cache_ll_l1_enable_bus(0, bus_mask);
	bus_mask = cache_ll_l1_get_bus(0, app_irom_vaddr_aligned, app_irom_size);
	cache_ll_l1_enable_bus(0, bus_mask);
#if CONFIG_MP_MAX_NUM_CPUS > 1
	bus_mask = cache_ll_l1_get_bus(1, app_drom_vaddr_aligned, app_drom_size);
	cache_ll_l1_enable_bus(1, bus_mask);
	bus_mask = cache_ll_l1_get_bus(1, app_irom_vaddr_aligned, app_irom_size);
	cache_ll_l1_enable_bus(1, bus_mask);
#endif

	/* ----------------------Enable Cache---------------- */
	cache_hal_enable(CACHE_TYPE_ALL);
}

void __start(void)
{
#ifdef CONFIG_BOOTLOADER_MCUBOOT
	size_t _partition_offset = FIXED_PARTITION_OFFSET(slot0_partition);
	uint32_t _app_irom_start = (_partition_offset + (uint32_t)&_image_irom_start);
	uint32_t _app_irom_size = (uint32_t)&_image_irom_size;
	uint32_t _app_irom_vaddr = ((uint32_t)&_image_irom_vaddr);

	uint32_t _app_drom_start = (_partition_offset + (uint32_t)&_image_drom_start);
	uint32_t _app_drom_size = (uint32_t)&_image_drom_size;
	uint32_t _app_drom_vaddr = ((uint32_t)&_image_drom_vaddr);
	uint32_t actual_mapped_len = 0;

	map_rom_segments(_app_drom_start, _app_drom_vaddr, _app_drom_size,
				_app_irom_start, _app_irom_vaddr, _app_irom_size);

	/* Show map segments continue using same log format as during MCUboot phase */
	BOOT_LOG_INF("DROM segment: paddr=%08Xh, vaddr=%08Xh, size=%05Xh (%6d) map",
		_app_drom_start, _app_drom_vaddr, _app_drom_size, _app_drom_size);
	BOOT_LOG_INF("IROM segment: paddr=%08Xh, vaddr=%08Xh, size=%05Xh (%6d) map\r\n",
		_app_irom_start, _app_irom_vaddr, _app_irom_size, _app_irom_size);
	esp_rom_uart_tx_wait_idle(0);
#endif
	__esp_platform_start();
}
