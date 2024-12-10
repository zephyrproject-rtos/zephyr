/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

/* SRAM0 (16kB) memory */
#define SRAM0_IRAM_START   DT_REG_ADDR(DT_NODELABEL(sram0))
#define SRAM0_SIZE         DT_REG_SIZE(DT_NODELABEL(sram0))
/* SRAM1 (384kB) memory */
#define SRAM1_DRAM_START   DT_REG_ADDR(DT_NODELABEL(sram1))
#define SRAM1_IRAM_START   (SRAM1_DRAM_START + IRAM_DRAM_OFFSET)
#define SRAM1_SIZE         DT_REG_SIZE(DT_NODELABEL(sram1))
/* ICache size is fixed to 16KB on ESP32-C3 */
#define ICACHE_SIZE        SRAM0_SIZE

/** Simplified memory map for the bootloader.
 *  Make sure the bootloader can load into main memory without overwriting itself.
 *
 *  ESP32-C3 ROM static data usage is as follows:
 *  - 0x3fccae00 - 0x3fcdc710: Shared buffers, used in UART/USB/SPI download mode only
 *  - 0x3fcdc710 - 0x3fcde710: PRO CPU stack, can be reclaimed as heap after RTOS startup
 *  - 0x3fcde710 - 0x3fce0000: ROM .bss and .data (not easily reclaimable)
 *
 *  The 2nd stage bootloader can take space up to the end of ROM shared
 *  buffers area (0x3fcdc710).
 */

/* The offset between Dbus and Ibus.
 * Used to convert between 0x403xxxxx and 0x3fcxxxxx addresses.
 */
#define IRAM_DRAM_OFFSET         0x700000
#define DRAM_BUFFERS_START       0x3fccae00
#define DRAM_BUFFERS_END         0x3fccc000
#define DRAM_STACK_START         0x3fcdc710
#define DRAM_ROM_BSS_DATA_START  0x3fcde710

/* Set the limit for the application runtime dynamic allocations */
#define DRAM_RESERVED_START      DRAM_BUFFERS_END

/* Base address used for calculating memory layout
 * counted from Dbus backwards and back to the Ibus
 */
#define BOOTLOADER_USER_DRAM_END DRAM_BUFFERS_START

/* For safety margin between bootloader data section and startup stacks */
#define BOOTLOADER_STACK_OVERHEAD      0x0
/* These lengths can be adjusted, if necessary: */
#define BOOTLOADER_DRAM_SEG_LEN        0x9800
#define BOOTLOADER_IRAM_SEG_LEN        0x9800
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x1400

/* Start of the lower region is determined by region size and the end of the higher region */
#define BOOTLOADER_IRAM_LOADER_SEG_END \
		(BOOTLOADER_USER_DRAM_END + BOOTLOADER_STACK_OVERHEAD + IRAM_DRAM_OFFSET)
#define BOOTLOADER_IRAM_LOADER_SEG_START \
		(BOOTLOADER_IRAM_LOADER_SEG_END - BOOTLOADER_IRAM_LOADER_SEG_LEN)
#define BOOTLOADER_IRAM_SEG_START \
		(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_LEN)
#define BOOTLOADER_DRAM_SEG_START \
		(BOOTLOADER_IRAM_SEG_START - IRAM_DRAM_OFFSET - BOOTLOADER_DRAM_SEG_LEN)

/* Flash */
#ifdef CONFIG_FLASH_SIZE
#define FLASH_SIZE         CONFIG_FLASH_SIZE
#else
#define FLASH_SIZE         0x400000
#endif

/* Cached memory */
#define CACHE_ALIGN        CONFIG_MMU_PAGE_SIZE
#define IROM_SEG_ORG       0x42000000
#define IROM_SEG_LEN       FLASH_SIZE
#define DROM_SEG_ORG       0x3c000000
#define DROM_SEG_LEN       FLASH_SIZE
