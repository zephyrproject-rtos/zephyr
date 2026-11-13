/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

/* LP-SRAM (32kB) memory
 * From HAL soc.h: SOC_RTC_IRAM_LOW = 0x2E000000, SOC_RTC_IRAM_HIGH = 0x2E008000
 */
#define LPSRAM_IRAM_START            DT_REG_ADDR(DT_NODELABEL(sramlp))
#define LPSRAM_SIZE                  DT_REG_SIZE(DT_NODELABEL(sramlp))
#define LPSRAM_RTC_START             DT_REG_ADDR(DT_NODELABEL(lp_rtc))
#define LPSRAM_RTC_SIZE              DT_REG_SIZE(DT_NODELABEL(lp_rtc))
#define ESP32_ULP_COPROC_RESERVE_MEM CONFIG_ESP32_ULP_COPROC_RESERVE_MEM
#define ESP32_ULP_SHARED_MEM_SIZE    DT_REG_SIZE(DT_NODELABEL(ulp_shm))
#define ESP32_ULP_SHARED_MEM_ADDR    DT_REG_ADDR(DT_NODELABEL(ulp_shm))

/* HP-SRAM (512kB) memory
 * From HAL soc.h: SOC_IRAM_LOW = 0x2F000000, SOC_IRAM_HIGH = 0x2F080000
 */
#define HPSRAM_START      DT_REG_ADDR(DT_NODELABEL(sramhp))
#define HPSRAM_SIZE       DT_REG_SIZE(DT_NODELABEL(sramhp))
#define HPSRAM_DRAM_START HPSRAM_START
#define HPSRAM_IRAM_START HPSRAM_START
#define ICACHE_SIZE       0x8000

/* ESP32-S31 ROM static data usage:
 * - 0x2F0696B0 - 0x2F07AFB0: Shared buffers, used in download mode only
 * - 0x2F07AFB0 - 0x2F07CFB0: ROM boot stack, reclaimable after startup
 * - 0x2F07CFB0 - 0x2F080000: Second CPU boot stack, ROM .bss and .data
 *
 * Images loaded by the ROM must end below the ROM boot stack.
 */
#define DRAM_SHARED_BUFFERS_END 0x2F07AFB0

/* Upper boundary of user-usable SRAM */
#define DRAM_USER_END DRAM_SHARED_BUFFERS_END

/* Safety margin between MCUboot segments and ROM stack */
#define BOOTLOADER_STACK_OVERHEAD 0x2000

#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x2000
#define BOOTLOADER_DRAM_LOADER_SEG_LEN 0x0C00

/* Upper limit of SRAM available for MCUboot bootloader segments */
#define BOOTLOADER_USER_DRAM_END (DRAM_SHARED_BUFFERS_END - BOOTLOADER_STACK_OVERHEAD)

/* MCUboot iram/dram segments sit below its loader segments. They overlap
 * the application image area, since only the loader segments must stay
 * intact while the application is loaded.
 */
#define BOOTLOADER_IRAM_LOADER_SEG_START (BOOTLOADER_USER_DRAM_END - BOOTLOADER_IRAM_LOADER_SEG_LEN)
#define BOOTLOADER_DRAM_LOADER_SEG_START                                                           \
	(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_DRAM_LOADER_SEG_LEN)
#define BOOTLOADER_IRAM_SEG_TARGET_LEN                                                             \
	((BOOTLOADER_DRAM_LOADER_SEG_START - (HPSRAM_START + ICACHE_SIZE)) / 4)
#define BOOTLOADER_IRAM_SEG_START                                                                  \
	ALIGN_UP(BOOTLOADER_DRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_TARGET_LEN, 0x100)
#define BOOTLOADER_IRAM_SEG_LEN   (BOOTLOADER_DRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_START)
#define BOOTLOADER_DRAM_SEG_LEN   BOOTLOADER_IRAM_SEG_LEN
#define BOOTLOADER_DRAM_SEG_START (BOOTLOADER_IRAM_SEG_START - BOOTLOADER_DRAM_SEG_LEN)

/* Flash */
#define FLASH_SIZE         DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
#define FLASH_BASE_ADDRESS DT_REG_ADDR(DT_CHOSEN(zephyr_flash))

/* Cached memory - ESP32-S31 uses unified I/D address space
 * From HAL ext_mem_defs.h: SOC_IRAM0_CACHE_ADDRESS_LOW = 0x40000000
 */
#define CACHE_ALIGN  CONFIG_MMU_PAGE_SIZE
#define IROM_SEG_ORG 0x40000000
#define IROM_SEG_LEN FLASH_SIZE
/* DROM shares the flash cache window with IROM and follows .text */
#define DROM_SEG_ORG IROM_SEG_ORG
#define DROM_SEG_LEN FLASH_SIZE

/* External RAM (PSRAM) cache window, mapped in its own MMU region */
#define EXTRAM_START DT_REG_ADDR(DT_NODELABEL(ext_ram))
