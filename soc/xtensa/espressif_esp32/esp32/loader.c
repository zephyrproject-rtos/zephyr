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

#include "debugpin.h"

#if defined(CONFIG_BOOTLOADER_MCUBOOT) || defined(CONFIG_SIMPLE_BOOT)

#include "debugled.h"

#include "bootloader_init.h"
#include "esp_rom_uart.h"
#include "esp_rom_sys.h"

#define BOOT_LOG_INF(_fmt, ...) \
	ets_printf("[" CONFIG_SOC_SERIES "] [INF] " _fmt "\n\r", ##__VA_ARGS__)

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

#ifdef CONFIG_SIMPLE_BOOT

#define IS_PADD(addr) (addr == 0)
#define IS_DRAM(addr) (addr >= SOC_DRAM_LOW && addr < SOC_DRAM_HIGH)
#define IS_IRAM(addr) (addr >= SOC_IRAM_LOW && addr < SOC_IRAM_HIGH)
#define IS_IROM(addr) (addr >= SOC_IROM_LOW && addr < SOC_IROM_HIGH)
#define IS_DROM(addr) (addr >= SOC_DROM_LOW && addr < SOC_DROM_HIGH)
#define IS_MMAP(addr) (IS_IROM(addr) || IS_DROM(addr))
#define IS_NONE(addr) (!IS_IROM(addr) && !IS_DROM(addr) \
			&& !IS_IRAM(addr) && !IS_DRAM(addr) && !IS_PADD(addr))

	esp_image_segment_header_t WORD_ALIGNED_ATTR segment_hdr;
	size_t offset = FIXED_PARTITION_OFFSET(boot_partition);
	unsigned int seg_count = 0;

	if (spi_flash_read(offset, &bootloader_image_hdr, sizeof(esp_image_header_t)) != ESP_OK) {
		ESP_LOGE(TAG, "failed to load bootloader image header!");
		abort();
	}
	offset += sizeof(esp_image_header_t);

	while (seg_count++ < 16) {

		if (spi_flash_read(offset, &segment_hdr, sizeof(esp_image_segment_header_t)) != ESP_OK) {
			ets_printf("failed to read segment header at %x\n", offset);
			abort();
		}

		if (IS_NONE(segment_hdr.load_addr)) {
			/* Total segment count is (seg_count - 1) */
			break;
		}

		BOOT_LOG_INF("offset %08x vma %08x len %-7u (%6xh) %s",
			offset + sizeof(esp_image_segment_header_t),
			segment_hdr.load_addr, segment_hdr.data_len, segment_hdr.data_len,
			IS_NONE(segment_hdr.load_addr) ? "unknown" :
			  IS_MMAP(segment_hdr.load_addr) ?
			    IS_IROM(segment_hdr.load_addr) ? "irom map" : "drom map" :
			      IS_PADD(segment_hdr.load_addr) ? "padding" :
			        IS_DRAM(segment_hdr.load_addr) ? "dram load" : "iram load");

		/* Fix the drom and irom LMA from the linker,
		 * which could be invalidated by elf2image */
		if (IS_DROM(segment_hdr.load_addr)) {
			_app_drom_start = offset + sizeof(esp_image_segment_header_t);
		}
		if (IS_IROM(segment_hdr.load_addr)) {
			_app_irom_start = offset + sizeof(esp_image_segment_header_t);
		}
		offset += sizeof(esp_image_segment_header_t) + segment_hdr.data_len;
	}
	if (seg_count == 0 || seg_count == 16) {
		ets_printf("Error occured\n");
		abort();
	}
#endif /* CONFIG_SIMPLE_BOOT */

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
	uint32_t drom_page_count = bootloader_cache_pages_to_map(
				_app_drom_size, _app_drom_vaddr);
	rc = cache_flash_mmu_set(0, 0, drom_vaddr_addr_aligned,
			_app_drom_start	& MMU_FLASH_MASK, 64, drom_page_count);
	rc |= cache_flash_mmu_set(1, 0, drom_vaddr_addr_aligned,
			_app_drom_start & MMU_FLASH_MASK, 64, drom_page_count);

	uint32_t irom_vaddr_addr_aligned = _app_irom_vaddr & MMU_FLASH_MASK;
	uint32_t irom_page_count = bootloader_cache_pages_to_map(
				_app_irom_size, _app_irom_vaddr);
	rc |= cache_flash_mmu_set(0, 0, irom_vaddr_addr_aligned,
			_app_irom_start	& MMU_FLASH_MASK, 64, irom_page_count);
	rc |= cache_flash_mmu_set(1, 0, irom_vaddr_addr_aligned,
			_app_irom_start	& MMU_FLASH_MASK, 64, irom_page_count);

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

ets_printf("drom_vaddr_addr_aligned = %x\n", drom_vaddr_addr_aligned);
ets_printf("drom_start = %x (offset = %x)\n", _app_drom_start & MMU_FLASH_MASK, _app_drom_start);
ets_printf("drom_page_count = %d\n", drom_page_count);
ets_printf("irom_vaddr_addr_aligned = %x\n", irom_vaddr_addr_aligned);
ets_printf("irom_start = %x (offset = %x)\n", _app_irom_start & MMU_FLASH_MASK, _app_irom_start);
ets_printf("irom_page_count = %d\n", irom_page_count);

	/* Show map segments continue using same log format as during MCUboot phase */
	BOOT_LOG_INF("DROM segment: paddr=%08Xh, vaddr=%08Xh, size=%05Xh (%6d) map",
		_app_drom_start, _app_drom_vaddr, _app_drom_size, _app_drom_size);
	BOOT_LOG_INF("IROM segment: paddr=%08Xh, vaddr=%08Xh, size=%05Xh (%6d) map\r\n",
		_app_irom_start, _app_irom_vaddr, _app_irom_size, _app_irom_size);
	esp_rom_uart_tx_wait_idle(0);
ets_printf("errx=%x\n", rc);
	return rc;
}
#endif /* CONFIG_BOOTLOADER_MCUBOOT || CONFIG_SIMPLE_BOOT */

void __start(void)
{
#ifdef CONFIG_SIMPLE_BOOT
	/* Init fundamental components */
	if (bootloader_init()) {
		ets_printf("HW init failed, aborting\n");
		abort();
	}
#endif /* CONFIG_SIMPLE_BOOT */

#if defined(CONFIG_BOOTLOADER_MCUBOOT) || defined(CONFIG_SIMPLE_BOOT)
	int err = map_rom_segments();

	if (err != 0) {
		ets_printf("Failed to setup XIP, aborting\n");
		abort();
	}
#endif /* CONFIG_BOOTLOADER || CONFIG_SIMPLE_BOOT */

	__esp_platform_start();
}
