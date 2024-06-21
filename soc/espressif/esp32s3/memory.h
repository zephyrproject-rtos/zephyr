/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

/* SRAM0 (64k), SRAM1 (416k), SRAM2 (64k) memories
 * Ibus and Dbus address space
 */
#define SRAM0_IRAM_START    0x40370000
#define SRAM0_SIZE          0x8000
#define SRAM1_DRAM_START    0x3fc88000
/* IRAM equivalent address where DRAM actually start */
#define SRAM1_IRAM_START    (SRAM0_IRAM_START + SRAM0_SIZE)
#define SRAM2_DRAM_START    0x3fcf0000
#define SRAM2_SIZE          0x10000

/** Simplified memory map for the bootloader.
 *  Make sure the bootloader can load into main memory without overwriting itself.
 *
 *  ESP32-S3 ROM static data usage is as follows:
 *  - 0x3fcd7e00 - 0x3fce9704: Shared buffers, used in UART/USB/SPI download mode only
 *  - 0x3fce9710 - 0x3fceb710: PRO CPU stack, can be reclaimed as heap after RTOS startup
 *  - 0x3fceb710 - 0x3fced710: APP CPU stack, can be reclaimed as heap after RTOS startup
 *  - 0x3fced710 - 0x3fcf0000: ROM .bss and .data (not easily reclaimable)
 *
 *  The 2nd stage bootloader can take space up to the end of ROM shared
 *  buffers area (0x3fce9704). For alignment purpose we shall use value (0x3fce9700).
 */

/* The offset between Dbus and Ibus.
 * Used to convert between 0x403xxxxx and 0x3fcxxxxx addresses.
 */
#define IRAM_DRAM_OFFSET         0x6f0000
#define DRAM_BUFFERS_START       0x3fcd7e00
#define DRAM_PROCPU_STACK_START  0x3fce9710
#define DRAM_STACK_START DRAM_PROCPU_STACK_START
#define DRAM_APPCPU_STACK_START  0x3fceb710
#define DRAM_ROM_BSS_DATA_START  0x3fcf0000

/* Base address used for calculating memory layout
 * counted from Dbus backwards and back to the Ibus
 */
#define BOOTLOADER_USER_DRAM_END DRAM_BUFFERS_START

/* For safety margin between bootloader data section and startup stacks */
#define BOOTLOADER_STACK_OVERHEAD      0x0
#define BOOTLOADER_DRAM_SEG_LEN        0x8000
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x1a00
#define BOOTLOADER_IRAM_SEG_LEN        0xa800

/* Start of the lower region is determined by region size and the end of the higher region */
#define BOOTLOADER_IRAM_LOADER_SEG_START (BOOTLOADER_USER_DRAM_END - BOOTLOADER_STACK_OVERHEAD + \
					IRAM_DRAM_OFFSET - BOOTLOADER_IRAM_LOADER_SEG_LEN)
#define BOOTLOADER_IRAM_SEG_START (BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_LEN)
#define BOOTLOADER_DRAM_SEG_END   (BOOTLOADER_IRAM_SEG_START - IRAM_DRAM_OFFSET)
#define BOOTLOADER_DRAM_SEG_START (BOOTLOADER_DRAM_SEG_END - BOOTLOADER_DRAM_SEG_LEN)

/* Flash */
#ifdef CONFIG_FLASH_SIZE
#define FLASH_SIZE         CONFIG_FLASH_SIZE
#else
#define FLASH_SIZE         0x800000
#endif

/* Cached memory */
#define CACHE_ALIGN        CONFIG_MMU_PAGE_SIZE
#define IROM_SEG_ORG       0x42000000
#define IROM_SEG_LEN       FLASH_SIZE
#define DROM_SEG_ORG       0x3c000000
#define DROM_SEG_LEN       FLASH_SIZE

/* AMP */
#ifdef CONFIG_SOC_ENABLE_APPCPU
#define APPCPU_IRAM_SIZE  CONFIG_ESP32S3_APPCPU_IRAM
#define APPCPU_DRAM_SIZE  CONFIG_ESP32S3_APPCPU_DRAM
#else
#define APPCPU_IRAM_SIZE  0
#define APPCPU_DRAM_SIZE  0
#endif
