/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

/* LP-SRAM (16kB) memory
 * From HAL soc.h: SOC_RTC_IRAM_LOW = 0x50000000, SOC_RTC_IRAM_HIGH = 0x50004000
 */
#define LPSRAM_IRAM_START            DT_REG_ADDR(DT_NODELABEL(sramlp))
#define LPSRAM_SIZE                  DT_REG_SIZE(DT_NODELABEL(sramlp))
#define LPSRAM_RTC_START             DT_REG_ADDR(DT_NODELABEL(lp_rtc))
#define LPSRAM_RTC_SIZE              DT_REG_SIZE(DT_NODELABEL(lp_rtc))
#define ESP32_ULP_COPROC_RESERVE_MEM CONFIG_ESP32_ULP_COPROC_RESERVE_MEM
#define ESP32_ULP_SHARED_MEM_SIZE    DT_REG_SIZE(DT_NODELABEL(ulp_shm))
#define ESP32_ULP_SHARED_MEM_ADDR    DT_REG_ADDR(DT_NODELABEL(ulp_shm))

/* HP-SRAM (384kB) memory
 * From HAL soc.h: SOC_IRAM_LOW = 0x40800000, SOC_IRAM_HIGH = 0x40860000
 */
#define HPSRAM_START      DT_REG_ADDR(DT_NODELABEL(sramhp))
#define HPSRAM_SIZE       DT_REG_SIZE(DT_NODELABEL(sramhp))
#define HPSRAM_DRAM_START HPSRAM_START
#define HPSRAM_IRAM_START HPSRAM_START
/* ESP32-C5 uses unified I/D cache */
#define ICACHE_SIZE       0x8000

/** Simplified memory map for the bootloader.
 *  Make sure the bootloader can load into main memory without overwriting itself.
 *
 *  ESP32-C5 ROM static data usage is as follows
 *  (from HAL soc.h: SOC_ROM_STACK_START = 0x4085e5a0):
 *  - 0x4084ad08 - 0x4085c610: Shared buffers, used in UART/USB/SPI download mode only
 *  - 0x4085c610 - 0x4085e610: PRO CPU stack, can be reclaimed as heap after RTOS startup
 *  - 0x4085e610 - 0x40860000: ROM .bss and .data (not easily reclaimable)
 *
 *  The 2nd stage bootloader can take space up to the end of ROM shared
 *  buffers area (0x4085c610).
 */

#define DRAM_SHARED_BUFFERS_START 0x4084ad08
#define DRAM_SHARED_BUFFERS_END   0x4085c610
#define DRAM_STACK_START          DRAM_SHARED_BUFFERS_END
#define DRAM_ROM_BSS_DATA_START   0x4085e610

/* Upper boundary of user-usable SRAM */
#define DRAM_USER_END DRAM_SHARED_BUFFERS_END

/* Safety margin between MCUboot segments and ROM stack */
#define BOOTLOADER_STACK_OVERHEAD      0x2000
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x3000

/* Upper limit of SRAM available for MCUboot bootloader segments */
#define BOOTLOADER_USER_DRAM_END (DRAM_SHARED_BUFFERS_END - BOOTLOADER_STACK_OVERHEAD)

#define BOOTLOADER_IRAM_LOADER_SEG_START (BOOTLOADER_USER_DRAM_END - BOOTLOADER_IRAM_LOADER_SEG_LEN)

/* MCUboot iram/dram segments: placed in upper half of SRAM, below iram_loader_seg.
 * On unified-address SoCs (C5, C6, H2) these are the same physical memory.
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
#ifdef CONFIG_FLASH_SIZE
#define FLASH_SIZE CONFIG_FLASH_SIZE
#else
#define FLASH_SIZE 0x800000
#endif

/* Cached memory - ESP32-C5 uses unified I/D address space
 * From HAL ext_mem_defs.h: SOC_IRAM0_CACHE_ADDRESS_LOW = 0x42000000
 */
#define CACHE_ALIGN  CONFIG_MMU_PAGE_SIZE
#define IROM_SEG_ORG 0x42000000
#define IROM_SEG_LEN FLASH_SIZE
#define DROM_SEG_ORG 0x42800000
#define DROM_SEG_LEN FLASH_SIZE

/* External RAM (PSRAM)
 * From HAL soc.h: SOC_EXTRAM_DATA_LOW = 0x42000000, SOC_EXTRAM_DATA_HIGH = 0x44000000
 * Shares the same bus as IROM/DROM (unified cache).
 */
#define EXTRAM_START 0x42000000
#define EXTRAM_SIZE  0x2000000
