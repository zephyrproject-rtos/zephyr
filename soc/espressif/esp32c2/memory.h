/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

/* SRAM0 (16kB) memory */
#define SRAM0_IRAM_START   DT_REG_ADDR(DT_NODELABEL(sram0))
#define SRAM0_SIZE         DT_REG_SIZE(DT_NODELABEL(sram0))

/* SRAM1 (256kB) memory */
#define SRAM1_IRAM_START   (SRAM1_DRAM_START + IRAM_DRAM_OFFSET)
#define SRAM1_DRAM_START   DT_REG_ADDR(DT_NODELABEL(sram1))
#define SRAM1_SIZE         DT_REG_SIZE(DT_NODELABEL(sram1))

/* ICache size is fixed to 16KB on ESP32-C2 */
#define ICACHE_SIZE        SRAM0_SIZE

/** Simplified memory map for the bootloader.
 *  Make sure the bootloader can load into main memory without overwriting itself.
 *
 *  ESP32-C2 ROM static data usage is as follows:
 *  - 0x3fccb264 - 0x3fcdcb70: Shared buffers, used in UART/USB/SPI download mode only
 *  - 0x3fcdcb70 - 0x3fcdeb70: PRO CPU stack, can be reclaimed as heap after RTOS startup
 *  - 0x3fcdeb70 - 0x3fce0000: ROM .bss and .data (not easily reclaimable)
 *
 *  The 2nd stage bootloader can take space up to the end of ROM shared
 *  buffers area (0x3fcdcb70).
 */

/* The offset between Dbus and Ibus.
 * Used to convert between 0x403xxxxx and 0x3fcxxxxx addresses.
 */
#define IRAM_DRAM_OFFSET         0x6e0000

#define DRAM_BUFFERS_START       0x3fccb264
#define DRAM_STACK_START         0x3fcdcb70
#define DRAM_ROM_BSS_DATA_START  0x3fcdeb70

#define DRAM_RESERVED_START      DRAM_STACK_START

/* For safety margin between bootloader data section and startup stacks */
#define BOOTLOADER_STACK_OVERHEAD      0x0
/* These lengths can be adjusted, if necessary: */
#define BOOTLOADER_DRAM_SEG_LEN        0xb000
#define BOOTLOADER_IRAM_SEG_LEN        0xc800
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x1400

/* Base address used for calculating memory layout
 * counted from Dbus backwards and back to the Ibus
 */
#define BOOTLOADER_USER_DRAM_END (DRAM_BUFFERS_START + BOOTLOADER_STACK_OVERHEAD)

/* Start of the lower region is determined by region size and the end of the higher region */
#define BOOTLOADER_IRAM_LOADER_SEG_START \
	(BOOTLOADER_USER_DRAM_END - BOOTLOADER_IRAM_LOADER_SEG_LEN + IRAM_DRAM_OFFSET)
#define BOOTLOADER_IRAM_SEG_START \
	(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_LEN)
#define BOOTLOADER_DRAM_SEG_START \
	(BOOTLOADER_IRAM_SEG_START - BOOTLOADER_DRAM_SEG_LEN - IRAM_DRAM_OFFSET)

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
