/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/storage/flash_map.h>
#include <esp_log.h>
#include <stdlib.h>

#include <esp32s2/rom/cache.h>
#include "soc/cache_memory.h"
#include "soc/extmem_reg.h"
#include <bootloader_flash_priv.h>
#include "hexdump.h"
#if defined(CONFIG_BOOTLOADER_MCUBOOT) || defined(CONFIG_SIMPLE_BOOT)

#include <esp_app_format.h>
#include <bootloader_init.h>

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

#ifndef CONFIG_SIMPLE_BOOT
	size_t _partition_offset = FIXED_PARTITION_OFFSET(slot0_partition);
#else
	size_t _partition_offset = FIXED_PARTITION_OFFSET(boot_partition);
#endif /* CONFIG_SIMPLE_BOOT */
	uint32_t _app_irom_start = _partition_offset + (uint32_t)&_image_irom_start;
	uint32_t _app_irom_size = (uint32_t)&_image_irom_size;
	uint32_t _app_irom_vaddr = (uint32_t)&_image_irom_vaddr;

	uint32_t _app_drom_start = _partition_offset + (uint32_t)&_image_drom_start;
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

		/* Fix the drom and irom LMA address from the linker,
		 * which was rearranged by the esptool elf2image */
		if (IS_DROM(segment_hdr.load_addr)) {
			_app_drom_start = offset + sizeof(esp_image_segment_header_t);
			ets_printf("DROM start addr. %08X on offset %6X\n",
					segment_hdr.load_addr, _app_drom_start);
		}
		if (IS_IROM(segment_hdr.load_addr)) {
			_app_irom_start = offset + sizeof(esp_image_segment_header_t);
			ets_printf("IROM start addr. %08X on offset %6X\n",
					segment_hdr.load_addr, _app_irom_start);
		}
		offset += sizeof(esp_image_segment_header_t) + segment_hdr.data_len;
	}
	if (seg_count == 0 || seg_count == 16) {
		ets_printf("Error occured\n");
		abort();
	}
	ets_printf("Total image segments: %d\n", seg_count - 1);
#endif /* CONFIG_SIMPLE_BOOT */

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

ets_printf("###### is extra map needed?   addr size check : %x > %x\n",
		(_app_irom_vaddr + _app_irom_size), IRAM1_ADDRESS_LOW);

	if ((_app_irom_vaddr + _app_irom_size) > IRAM1_ADDRESS_LOW) {
		ets_printf("####### EXTRA MAPPING S2 ###########\n");
		rc |= esp_rom_Cache_Ibus_MMU_Set(MMU_ACCESS_FLASH, IRAM0_ADDRESS_LOW, 0, 64, 64, 1);
		rc |= esp_rom_Cache_Ibus_MMU_Set(MMU_ACCESS_FLASH, IRAM1_ADDRESS_LOW, 0, 64, 64, 1);
		REG_CLR_BIT(EXTMEM_PRO_ICACHE_CTRL1_REG, EXTMEM_PRO_ICACHE_MASK_IRAM1);
	}

	rc |= esp_rom_Cache_Ibus_MMU_Set(MMU_ACCESS_FLASH, _app_irom_vaddr & 0xffff0000,
				_app_irom_start & 0xffff0000, 64, irom_page_count, 0);

	REG_CLR_BIT(EXTMEM_PRO_ICACHE_CTRL1_REG, (EXTMEM_PRO_ICACHE_MASK_IRAM0) |
				(EXTMEM_PRO_ICACHE_MASK_IRAM1 & 0) | EXTMEM_PRO_ICACHE_MASK_DROM0);

	esp_rom_Cache_Resume_ICache(autoload);

	/* We dont want to print duplicit info during the startup,
	 * so we exclude this in case we are using Simple Boot
	 */
	/* Show map segments continue using same log format as during MCUboot phase */
	BOOT_LOG_INF("IROM segment: paddr=%08Xh, vaddr=%08Xh, size=%05Xh (%6d) map",
		_app_irom_start & 0xffff0000, _app_irom_vaddr & 0xffff0000,
		_app_irom_size, _app_irom_size);
	BOOT_LOG_INF("DROM segment: paddr=%08Xh, vaddr=%08Xh, size=%05Xh (%6d) map\r\n",
		_app_drom_start & 0xffff0000, _app_drom_vaddr & 0xffff0000,
		_app_drom_size, _app_drom_size);
	esp_rom_uart_tx_wait_idle(0);

//ets_printf("DROM dump\n");
//	hexdump("drom", (_app_drom_vaddr & 0xffff0000) + 0x000, 0x100);
//	hexdump("drom", (_app_drom_vaddr & 0xffff0000) + 0x1000, 0x400);
//ets_printf("%s:%d\n",__func__,__LINE__);
	return rc;
}
#endif /* CONFIG_BOOTLOADER_MCUBOOT || CONFIG_SIMPLE_BOOT */

void __start(void)
{
#ifdef CONFIG_SIMPLE_BOOT
	/* Init fundamental components before mapping ram segments */
	if (bootloader_init()) {
		ets_printf("\nABORT: hw init failed!\n");
		abort();
	}
#endif /* CONFIG_SIMPLE_BOOT */

#if defined(CONFIG_BOOTLOADER_MCUBOOT) || defined(CONFIG_SIMPLE_BOOT)
	int err = map_rom_segments();

	if (err != 0) {
		ets_printf("Failed to setup XIP, aborting\n");
		abort();
	}
#endif /* CONFIG_BOOTLOADER_MCUBOOT || CONFIG_SIMPLE_BOOT */

ets_printf("%s:%d\n",__func__,__LINE__);
	__esp_platform_start();
}
