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
 * The L2 cache is carved out of HP SRAM. Its location depends on the
 * silicon revision:
 *   rev >= v3.0: L2 cache sits at the LOW end of SRAM, so usable SRAM
 *                starts after it (HPSRAM_BASE + L2_CACHE_SIZE).
 *   rev <  v3.0: L2 cache sits at the HIGH end of SRAM, so usable SRAM
 *                ends where the cache begins; it starts above the low ROM
 *                reserved region (DRAM_ROM_RESERVED_END).
 */
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
 *  Pre-v3 silicon (rev < v3.0) differs in two ways: the L2 cache is carved
 *  from the TOP of SRAM instead of the bottom, and the ROM reserves its
 *  static data low, in 0x4ff3cfc0 - 0x4ff40000 (from HAL soc.h:
 *  SOC_ROM_STACK_START = 0x4ff3cfc0, SOC_DIRAM_ROM_RESERVE_HIGH =
 *  0x4ff40000). Usable SRAM therefore runs from the top of that ROM reserve
 *  up to the base of the top-of-SRAM cache:
 *  - 0x4ff40000 - (0x4ffc0000 - L2_CACHE_SIZE): available for user code
 *  - top L2_CACHE_SIZE bytes of SRAM: L2 cache
 *
 *  The 2nd stage bootloader can take space up to the ROM stack area.
 */

#if defined(CONFIG_SOC_ESP32P4_REV_1_3)
#define DRAM_ROM_RESERVED_START 0x4ff3cfc0
#define DRAM_ROM_RESERVED_END   0x4ff40000
#else
#define DRAM_ROM_RESERVED_START 0x4ffbcfc0
#define DRAM_ROM_RESERVED_END   0x4ffc0000
#endif

#define HPSRAM_BASE       DT_REG_ADDR(DT_NODELABEL(sramhp))
#define HPSRAM_TOTAL_SIZE DT_REG_SIZE(DT_NODELABEL(sramhp))
#define L2_CACHE_SIZE     CONFIG_ESP32_CACHE_L2_SIZE
#if defined(CONFIG_SOC_ESP32P4_REV_1_3)
#define HPSRAM_START DRAM_ROM_RESERVED_END
#else
#define HPSRAM_START (HPSRAM_BASE + L2_CACHE_SIZE)
#endif
#define HPSRAM_DRAM_START HPSRAM_START
#define HPSRAM_IRAM_START HPSRAM_START
#define ICACHE_SIZE       0x8000

/* Upper boundary of user-usable SRAM. On pre-v3 silicon the L2 cache sits at
 * the top of SRAM, so usable RAM ends where the cache begins. On v3.x the
 * cache is at the bottom, so usable RAM ends at the ROM reserved region.
 */
#if defined(CONFIG_SOC_ESP32P4_REV_1_3)
#define DRAM_USER_END (HPSRAM_BASE + HPSRAM_TOTAL_SIZE - L2_CACHE_SIZE)
#else
#define DRAM_USER_END DRAM_ROM_RESERVED_START
#endif

/* Stack pointer for early startup (__start) */
#define DRAM_STACK_START DRAM_USER_END

/* Safety margin between MCUboot segments and ROM stack */
#define BOOTLOADER_STACK_OVERHEAD      0x2000
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x3000

/* Upper limit of SRAM available for MCUboot bootloader segments */
#define BOOTLOADER_USER_DRAM_END (DRAM_USER_END - BOOTLOADER_STACK_OVERHEAD)

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
