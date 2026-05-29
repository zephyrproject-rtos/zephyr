/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

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

#define DRAM_SHARED_BUFFERS_START       0x3fccb264
#define DRAM_SHARED_BUFFERS_END         0x3fcdcb70
#define DRAM_STACK_START         DRAM_SHARED_BUFFERS_END
#define DRAM_ROM_BSS_DATA_START  0x3fcdeb70

/* Upper boundary of user-usable SRAM */
#define DRAM_USER_END      DRAM_SHARED_BUFFERS_END

/* Safety margin between MCUboot segments and ROM stack */
#define BOOTLOADER_STACK_OVERHEAD      0x2000
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x1400

/* Upper limit of SRAM available for MCUboot bootloader segments */
#define BOOTLOADER_USER_DRAM_END (DRAM_SHARED_BUFFERS_END - BOOTLOADER_STACK_OVERHEAD)

#define BOOTLOADER_IRAM_LOADER_SEG_START \
	(BOOTLOADER_USER_DRAM_END - BOOTLOADER_IRAM_LOADER_SEG_LEN + IRAM_DRAM_OFFSET)

/* MCUboot iram/dram segments: stacked in upper SRAM, below iram_loader_seg.
 * On split-bus SoCs, iram_seg and dram_seg MUST occupy separate physical
 * SRAM regions (IRAM and DRAM buses map to the same physical memory).
 * Layout (in physical/DRAM space, top-down):
 *   iram_loader_seg  (IRAM bus)
 *   iram_seg         (IRAM bus, 256-byte aligned start)
 *   dram_seg         (DRAM bus)
 */
#define BOOTLOADER_IRAM_SEG_TARGET_LEN \
	((BOOTLOADER_IRAM_LOADER_SEG_START - IRAM_DRAM_OFFSET - \
	  (SRAM1_DRAM_START + ICACHE_SIZE)) / 4)
#define BOOTLOADER_IRAM_SEG_START \
	ALIGN_UP(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_TARGET_LEN, 0x100)
#define BOOTLOADER_IRAM_SEG_LEN \
	(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_START)
#define BOOTLOADER_DRAM_SEG_LEN   BOOTLOADER_IRAM_SEG_LEN
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
