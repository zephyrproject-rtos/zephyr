/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

/* SRAM0 (32k) with adjacted SRAM1 (288k)
 * Ibus and Dbus address space
 */
#define SRAM_IRAM_START  (SRAM_DRAM_START + IRAM_DRAM_OFFSET)
#define SRAM_DRAM_START  DT_REG_ADDR(DT_NODELABEL(sram0))
#define SRAM_CACHE_SIZE  (CONFIG_ESP32S2_INSTRUCTION_CACHE_SIZE + CONFIG_ESP32S2_DATA_CACHE_SIZE)

/* RTC SLOW RAM (8k) */
#define RCT_SLOW_RAM_START DT_REG_ADDR(DT_NODELABEL(rtc_slow_ram))
#define RCT_SLOW_RAM_SIZE  DT_REG_SIZE(DT_NODELABEL(rtc_slow_ram))

/* RTC FAST RAM (8k) */
#define RCT_FAST_RAM_START        DT_REG_ADDR(DT_NODELABEL(rtc_fast_ram))
#define RCT_FAST_RAM_SIZE         DT_REG_SIZE(DT_NODELABEL(rtc_fast_ram))
#define RTC_FAST_IRAM_DRAM_OFFSET 0xd2000;

/** Simplified memory map for the bootloader.
 *  Make sure the bootloader can load into main memory without overwriting itself.
 *
 *  ESP32-S2 ROM static data usage is as follows:
 *  - 0x3ffeab00 - 0x3fffc410: Shared buffers, used in UART/USB/SPI download mode only
 *  - 0x3fffc410 - 0x3fffe710: CPU stack, can be reclaimed as heap after RTOS startup
 *  - 0x3fffe710 - 0x40000000: ROM .bss and .data (not easily reclaimable)
 *
 *  The 2nd stage bootloader can take space up to the end of ROM shared
 *  buffers area (0x3fffc410).
 */

/* The offset between Dbus and Ibus.
 * Used to convert between 0x4002xxxx and 0x3ffbxxxx addresses.
 */
#define IRAM_DRAM_OFFSET         0x70000
#define DRAM_SHARED_BUFFERS_START       0x3ffea400
#define DRAM_SHARED_BUFFERS_END         0x3fffc410
#define DRAM_STACK_START 0x3fffc410
#define DRAM_ROM_BSS_DATA_START  0x3fffe710

/* Safety margin between MCUboot segments and ROM stack */
#define BOOTLOADER_STACK_OVERHEAD      0x2000
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x3000

/* Upper boundary of user-usable SRAM */
#define DRAM_USER_END      DRAM_SHARED_BUFFERS_END

/* Upper limit of SRAM available for MCUboot bootloader segments */
#define BOOTLOADER_USER_DRAM_END (DRAM_SHARED_BUFFERS_END - BOOTLOADER_STACK_OVERHEAD)

#define BOOTLOADER_IRAM_LOADER_SEG_START \
	(BOOTLOADER_USER_DRAM_END - BOOTLOADER_IRAM_LOADER_SEG_LEN + IRAM_DRAM_OFFSET)

/* MCUboot iram/dram segments: stacked in upper SRAM, below iram_loader_seg.
 * On Xtensa split-bus SoCs, iram_seg and dram_seg MUST occupy separate physical
 * SRAM regions (IRAM and DRAM buses map to the same physical memory).
 * Layout (in physical/DRAM space, top-down):
 *   iram_loader_seg  (IRAM bus)
 *   iram_seg         (IRAM bus, 1024-byte aligned start for Xtensa VECBASE)
 *   dram_seg         (DRAM bus)
 */
#define BOOTLOADER_IRAM_SEG_TARGET_LEN \
	((BOOTLOADER_IRAM_LOADER_SEG_START - IRAM_DRAM_OFFSET - \
	  (SRAM_DRAM_START + SRAM_CACHE_SIZE)) / 4)
#define BOOTLOADER_IRAM_SEG_START \
	ALIGN_UP(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_TARGET_LEN, 0x400)
#define BOOTLOADER_IRAM_SEG_LEN \
	(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_START)
#define BOOTLOADER_DRAM_SEG_LEN   BOOTLOADER_IRAM_SEG_LEN
#define BOOTLOADER_DRAM_SEG_START \
	(BOOTLOADER_IRAM_SEG_START - IRAM_DRAM_OFFSET - BOOTLOADER_DRAM_SEG_LEN)

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
