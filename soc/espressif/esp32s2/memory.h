/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

/* SRAM0 (32k) with adjacted SRAM1 (288k)
 * Ibus and Dbus address space
 */
#define SRAM_IRAM_START  (SRAM_DRAM_START + IRAM_DRAM_OFFSET)
#define SRAM_DRAM_START  DT_REG_ADDR(DT_NODELABEL(sram0))
#define SRAM_CACHE_SIZE  (CONFIG_ESP32S2_INSTRUCTION_CACHE_SIZE + CONFIG_ESP32S2_DATA_CACHE_SIZE)

/** Simplified memory map for the bootloader.
 *  Make sure the bootloader can load into main memory without overwriting itself.
 *
 *  ESP32-S2 ROM static data usage is as follows:
 *  - 0x3ffeab00 - 0x3fffc410: Shared buffers, used in UART/USB/SPI download mode only
 *  - 0x3fffc410 - 0x3fffe710: CPU stack, can be reclaimed as heap after RTOS startup
 *  - 0x3fffe710 - 0x40000000: ROM .bss and .data (not easily reclaimable)
 *
 *  The 2nd stage bootloader can take space up to the end of ROM shared
 *  buffers area (0x3fffc410). For alignment purpose we shall use value (0x3fce9700).
 */

/* The offset between Dbus and Ibus.
 * Used to convert between 0x4002xxxx and 0x3ffbxxxx addresses.
 */
#define IRAM_DRAM_OFFSET         0x70000
#define DRAM_BUFFERS_START       0x3ffea400
#define DRAM_BUFFERS_END         0x3fffc410
#define DRAM_ROM_CPU_STACK_START 0x3fffc410
#define DRAM_ROM_BSS_DATA_START  0x3fffe710

/* For safety margin between bootloader data section and startup stacks */
#define BOOTLOADER_STACK_OVERHEAD      0x0
#define BOOTLOADER_DRAM_SEG_LEN        0x8000
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x3000
#define BOOTLOADER_IRAM_SEG_LEN        0xa000

/* Set the limit for the application runtime dynamic allocations */
#define DRAM_RESERVED_START      DRAM_BUFFERS_END

/* Base address used for calculating memory layout
 * counted from Dbus backwards and back to the Ibus
 */
#define BOOTLOADER_USER_DRAM_END (DRAM_BUFFERS_START - BOOTLOADER_STACK_OVERHEAD)

/* Start of the lower region is determined by region size and the end of the higher region */
#define BOOTLOADER_IRAM_LOADER_SEG_START \
	(BOOTLOADER_USER_DRAM_END - BOOTLOADER_IRAM_LOADER_SEG_LEN + IRAM_DRAM_OFFSET)
#define BOOTLOADER_IRAM_SEG_START (BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_LEN)
#define BOOTLOADER_DRAM_SEG_START \
	(BOOTLOADER_IRAM_SEG_START - BOOTLOADER_DRAM_SEG_LEN - IRAM_DRAM_OFFSET)

/* Cached memories */
#define ICACHE0_START DT_REG_ADDR(DT_NODELABEL(icache0))
#define ICACHE0_SIZE  DT_REG_SIZE(DT_NODELABEL(icache0))
#define DCACHE0_START DT_REG_ADDR(DT_NODELABEL(dcache0))
#define DCACHE0_SIZE  DT_REG_SIZE(DT_NODELABEL(dcache0))
#define DCACHE1_START DT_REG_ADDR(DT_NODELABEL(dcache1))
#define DCACHE1_SIZE  DT_REG_SIZE(DT_NODELABEL(dcache1))

#define CACHE_ALIGN       CONFIG_MMU_PAGE_SIZE

/* Flash */
#ifdef CONFIG_FLASH_SIZE
#define FLASH_SIZE        CONFIG_FLASH_SIZE
#else
#define FLASH_SIZE        0x400000
#endif
