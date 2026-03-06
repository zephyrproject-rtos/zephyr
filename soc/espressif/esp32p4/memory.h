/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

/* LP-SRAM (32kB) memory
 * From HAL soc.h: SOC_RTC_IRAM_LOW = 0x50108000, SOC_RTC_IRAM_HIGH = 0x50110000
 */
#define LPSRAM_IRAM_START            DT_REG_ADDR(DT_NODELABEL(sramlp))
#define LPSRAM_SIZE                  DT_REG_SIZE(DT_NODELABEL(sramlp))
#define LPSRAM_RTC_START             DT_REG_ADDR(DT_NODELABEL(lp_rtc))
#define LPSRAM_RTC_SIZE              DT_REG_SIZE(DT_NODELABEL(lp_rtc))
#define ESP32_ULP_COPROC_RESERVE_MEM CONFIG_ESP32_ULP_COPROC_RESERVE_MEM
#define ESP32_ULP_SHARED_MEM_SIZE    DT_REG_SIZE(DT_NODELABEL(ulp_shm))
#define ESP32_ULP_SHARED_MEM_ADDR    DT_REG_ADDR(DT_NODELABEL(ulp_shm))

/* HP-SRAM (768kB) memory
 * From HAL soc.h: SOC_IRAM_LOW = 0x4ff00000, SOC_IRAM_HIGH = 0x4ffc0000
 *
 * The L2 cache occupies the beginning of SRAM (default 128KB = 0x20000).
 * Usable SRAM starts after the L2 cache region.
 */
#define HPSRAM_BASE       DT_REG_ADDR(DT_NODELABEL(sramhp))
#define HPSRAM_TOTAL_SIZE DT_REG_SIZE(DT_NODELABEL(sramhp))
#define L2_CACHE_SIZE     CONFIG_ESP32_CACHE_L2_SIZE
#define HPSRAM_START      (HPSRAM_BASE + L2_CACHE_SIZE)
#define HPSRAM_SIZE       (HPSRAM_TOTAL_SIZE - L2_CACHE_SIZE)
#define HPSRAM_DRAM_START HPSRAM_START
#define HPSRAM_IRAM_START HPSRAM_START
#define ICACHE_SIZE       0x8000

/** Simplified memory map for the bootloader.
 *  Make sure the bootloader can load into main memory without overwriting itself.
 *
 *  ESP32-P4 ROM static data usage (rev >= v3, ECO5):
 *  (from HAL soc.h: SOC_ROM_STACK_START_REV2 = 0x4ffbcfc0)
 *  - 0x4ff00000 - 0x4ff20000: L2 cache memory (128KB default)
 *  - 0x4ff20000 - 0x4ffbcfc0: Available for user code
 *  - 0x4ffbcfc0 - 0x4ffbefc0: ROM stack (CPU0 + CPU1)
 *  - 0x4ffbefc0 - 0x4ffc0000: ROM .bss and .data
 *
 *  The 2nd stage bootloader can take space up to the ROM stack area.
 */

#define DRAM_ROM_RESERVED_START 0x4ffbcfc0
#define DRAM_ROM_RESERVED_END   0x4ffc0000

/* Upper boundary of user-usable SRAM */
#define DRAM_USER_END DRAM_ROM_RESERVED_START

/* Stack pointer for early startup (__start) */
#define DRAM_STACK_START DRAM_ROM_RESERVED_START

/* Safety margin between MCUboot segments and ROM stack */
#define BOOTLOADER_STACK_OVERHEAD      0x2000
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x3000

/* Upper limit of SRAM available for MCUboot bootloader segments */
#define BOOTLOADER_USER_DRAM_END (DRAM_ROM_RESERVED_START - BOOTLOADER_STACK_OVERHEAD)

#define BOOTLOADER_IRAM_LOADER_SEG_START (BOOTLOADER_USER_DRAM_END - BOOTLOADER_IRAM_LOADER_SEG_LEN)

/* MCUboot iram/dram segments: placed in upper half of SRAM, below iram_loader_seg.
 * On unified-address SoCs (P4, C5, C6, H2) these are the same physical memory.
 * The lower half is reserved for the application image.
 */
#define BOOTLOADER_IRAM_SEG_TARGET_LEN                                                             \
	((BOOTLOADER_IRAM_LOADER_SEG_START - (HPSRAM_START + ICACHE_SIZE)) / 4)
#define BOOTLOADER_IRAM_SEG_START                                                                  \
	ALIGN_UP(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_TARGET_LEN, 0x100)
#define BOOTLOADER_IRAM_SEG_LEN   (BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_START)
#define BOOTLOADER_DRAM_SEG_LEN   BOOTLOADER_IRAM_SEG_LEN
#define BOOTLOADER_DRAM_SEG_START (BOOTLOADER_IRAM_SEG_START - BOOTLOADER_DRAM_SEG_LEN)

/* Flash */
#define FLASH_SIZE         DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
#define FLASH_BASE_ADDRESS DT_REG_ADDR(DT_CHOSEN(zephyr_flash))

/* Cached memory - ESP32-P4 uses unified I/D address space
 * From HAL ext_mem_defs.h: SOC_IRAM0_CACHE_ADDRESS_LOW = 0x40000000
 */
#define CACHE_ALIGN  CONFIG_MMU_PAGE_SIZE
#define IROM_SEG_ORG 0x40000000
#define IROM_SEG_LEN FLASH_SIZE
/* DROM shares the unified-cache linear address space with IROM. Placing
 * drom0_0_seg at the same origin lets the linker emit .flash.rodata
 * immediately after .text, so the MMU allocator's linear free_head
 * advance (irom_len + drom_len) matches the actual reserved virtual
 * range. This avoids a gap that PSRAM mapping could overrun.
 */
#define DROM_SEG_ORG IROM_SEG_ORG
#define DROM_SEG_LEN FLASH_SIZE

/* External PSRAM - P4 has dedicated bus at 0x48000000
 * From HAL soc.h: SOC_EXTRAM_LOW = 0x48000000, SOC_EXTRAM_HIGH = 0x4c000000
 */
#define EXTRAM_SEG_ORG 0x48000000
