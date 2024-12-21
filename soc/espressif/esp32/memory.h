/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

/* SRAM0 (192kB)  instruction cache+memory */
#define SRAM0_IRAM_START    DT_REG_ADDR(DT_NODELABEL(sram0))
#define SRAM0_CACHE_SIZE    0x10000
#define SRAM0_SIZE          DT_REG_SIZE(DT_NODELABEL(sram0))

/* SRAM1 (128kB) instruction/data memory */
#define SRAM1_IRAM_START      (SRAM0_IRAM_START + SRAM0_SIZE)
#define SRAM1_DRAM_START      DT_REG_ADDR(DT_NODELABEL(sram1))
#define SRAM1_SIZE            DT_REG_SIZE(DT_NODELABEL(sram1))
#define SRAM1_DRAM_END        (SRAM1_DRAM_START + SRAM1_SIZE)
#define SRAM1_RESERVED_SIZE   0x8000
#define SRAM1_DRAM_USER_START (SRAM1_DRAM_START + SRAM1_RESERVED_SIZE)
#define SRAM1_USER_SIZE       (0x40000000 - SRAM1_DRAM_USER_START)

/* SRAM2 (200kB) data memory */
#define SRAM2_DRAM_START      DT_REG_ADDR(DT_NODELABEL(sram2))
#define SRAM2_DRAM_SIZE       DT_REG_SIZE(DT_NODELABEL(sram2))
#define SRAM2_DRAM_SHM_SIZE   0x2000
#define SRAM2_DRAM_END        (SRAM2_DRAM_START + SRAM2_DRAM_SIZE)
#define SRAM2_DRAM_USER_START (SRAM2_DRAM_START + SRAM2_DRAM_SHM_SIZE)
#define SRAM2_DRAM_USER_SIZE  (SRAM2_DRAM_END - SRAM2_DRAM_USER_START)

/** Simplified memory map for the bootloader.
 *  Make sure the bootloader can load into main memory without overwriting itself.
 *
 *  ESP32 ROM static data usage is as follows:
 *  - 0x3ffae000 - 0x3ffb0000 (Reserved: data memory for ROM functions)
 *  - 0x3ffb0000 - 0x3ffe0000 (RAM bank 1 for application usage)
 *  - 0x3ffe0000 - 0x3ffe0440 (Reserved: data memory for ROM PRO CPU)
 *  - 0x3ffe3f20 - 0x3ffe4350 (Reserved: data memory for ROM APP CPU)
 *  - 0x3ffe4350 - 0x3ffe5230 (BT shm buffers)
 *  - 0x3ffe8000 - 0x3fffffff (RAM bank 2 for application usage)
 */

#define DRAM1_PROCPU_RESERVED_START 0x3ffe0000
#define DRAM1_APPCPU_RESERVED_START 0x3ffe3f20
#define DRAM1_BT_SHM_BUFFERS_START  0x3ffe4350
#define DRAM1_BT_SHM_BUFFERS_END    0x3ffe5230

/* The address is a limit set manually for AMP build */
#define DRAM1_AMP_SHM_BUFFERS_END   0x3ffe9800

/* Convert IRAM address to its DRAM counterpart in SRAM1 memory */
#define SRAM1_IRAM_DRAM_CALC(addr_iram) (SRAM1_SIZE - (addr_iram - SRAM1_IRAM_START) + \
					SRAM1_DRAM_START)
/* Convert DRAM address to its IRAM counterpart in SRAM1 memory */
#define SRAM1_DRAM_IRAM_CALC(addr_dram) (SRAM1_SIZE - (addr_dram - SRAM1_DRAM_START) + \
					SRAM1_IRAM_START)

/* Set bootloader segments size */
#define BOOTLOADER_DRAM_SEG_LEN        0x7a00
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x4000
#define BOOTLOADER_IRAM_SEG_LEN        0xa000

/* Start of the lower region is determined by region size and the end of the higher region */
#define BOOTLOADER_DRAM_SEG_START  0x3ffe8000
#define BOOTLOADER_DRAM_SEG_END    (BOOTLOADER_DRAM_SEG_START + BOOTLOADER_DRAM_SEG_LEN)
#define BOOTLOADER_IRAM_LOADER_SEG_START 0x40078000
#define BOOTLOADER_IRAM_SEG_START  0x400a0000

/* The `USER_IRAM_END` represents the end of staticaly allocated memory.
 * This address is where 2nd stage bootloader starts allocating memory.
 * The `iram_loader_seg` which is the last memory the bootloader runs from
 * resides in the SRAM0 'cache' area, the `user_iram_end` applies for
 * all build cases - Simple boot and the MCUboot application.
 */
#if defined(CONFIG_SOC_ENABLE_APPCPU) || defined(CONFIG_SOC_ESP32_APPCPU)
#define USER_IRAM_END SRAM1_DRAM_IRAM_CALC(DRAM1_AMP_SHM_BUFFERS_END)
#else
#define USER_IRAM_END SRAM1_DRAM_IRAM_CALC(SRAM1_DRAM_USER_START)
#endif

/* AMP memory */
#if defined(CONFIG_SOC_ENABLE_APPCPU) || defined(CONFIG_SOC_ESP32_APPCPU)
#define APPCPU_IRAM_SIZE CONFIG_ESP_APPCPU_IRAM_SIZE
#define APPCPU_DRAM_SIZE CONFIG_ESP_APPCPU_DRAM_SIZE
#else
#define APPCPU_IRAM_SIZE 0
#define APPCPU_DRAM_SIZE 0
#endif

#define APPCPU_SRAM_SIZE (APPCPU_IRAM_SIZE + APPCPU_DRAM_SIZE)

/* Flash */
#ifdef CONFIG_FLASH_SIZE
#define FLASH_SIZE          CONFIG_FLASH_SIZE
#else
#define FLASH_SIZE          0x400000
#endif

/* Cached memories */
#define CACHE_ALIGN        CONFIG_MMU_PAGE_SIZE
#define IROM_SEG_ORG       0x400d0000
#define IROM_SEG_LEN       (FLASH_SIZE - 0x1000)
#define DROM_SEG_ORG       0x3f400000
#define DROM_SEG_LEN       (FLASH_SIZE - 0x1000)
