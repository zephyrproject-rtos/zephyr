/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

/* LP-SRAM (16kB) memory */
#define LPSRAM_IRAM_START      DT_REG_ADDR(DT_NODELABEL(sramlp))
#define LPSRAM_SIZE            DT_REG_SIZE(DT_NODELABEL(sramlp))
#define ULP_SHARED_MEM_SIZE    DT_REG_SIZE(DT_NODELABEL(shmlp))
#define ULP_SHARED_MEM_ADDR    DT_REG_ADDR(DT_NODELABEL(shmlp))
#define ULP_COPROC_RESERVE_MEM (0x4000)

/* HP-SRAM (512kB) memory */
#define HPSRAM_START          DT_REG_ADDR(DT_NODELABEL(sramhp))
#define HPSRAM_SIZE           DT_REG_SIZE(DT_NODELABEL(sramhp))
#define HPSRAM_DRAM_START     HPSRAM_START
#define HPSRAM_IRAM_START     HPSRAM_START
/* ICache size is fixed to 32KB on ESP32-C6 */
#define ICACHE_SIZE           0x8000

/** Simplified memory map for the bootloader.
 *  Make sure the bootloader can load into main memory without overwriting itself.
 *
 *  ESP32-C6 ROM static data usage is as follows:
 *  - 0x4086ad08 - 0x4087c610: Shared buffers, used in UART/USB/SPI download mode only
 *  - 0x4087c610 - 0x4087e610: PRO CPU stack, can be reclaimed as heap after RTOS startup
 *  - 0x4087e610 - 0x40880000: ROM .bss and .data (not easily reclaimable)
 *
 *  The 2nd stage bootloader can take space up to the end of ROM shared
 *  buffers area (0x4087c610).
 */

#define DRAM_SHARED_BUFFERS_START      0x4086ad08
#define DRAM_SHARED_BUFFERS_END        0x4087c610
#define DRAM_STACK_START        DRAM_SHARED_BUFFERS_END
#define DRAM_ROM_BSS_DATA_START 0x4087e610

/* Upper boundary of user-usable SRAM */
#define DRAM_USER_END DRAM_SHARED_BUFFERS_END

/* Safety margin between MCUboot segments and ROM stack */
#define BOOTLOADER_STACK_OVERHEAD      0x2000
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x3000

/* Upper limit of SRAM available for MCUboot bootloader segments */
#define BOOTLOADER_USER_DRAM_END (DRAM_SHARED_BUFFERS_END - BOOTLOADER_STACK_OVERHEAD)

#define BOOTLOADER_IRAM_LOADER_SEG_START (BOOTLOADER_USER_DRAM_END - BOOTLOADER_IRAM_LOADER_SEG_LEN)

/* MCUboot iram/dram segments: placed in upper half of SRAM, below iram_loader_seg.
 * On unified-address SoCs (C6, H2) these are the same physical memory.
 * The lower half is reserved for the application image.
 */
#define BOOTLOADER_IRAM_SEG_TARGET_LEN \
	((BOOTLOADER_IRAM_LOADER_SEG_START - (HPSRAM_START + ICACHE_SIZE)) / 4)
#define BOOTLOADER_IRAM_SEG_START \
	ALIGN_UP(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_TARGET_LEN, 0x100)
#define BOOTLOADER_IRAM_SEG_LEN \
	(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_START)
#define BOOTLOADER_DRAM_SEG_LEN   BOOTLOADER_IRAM_SEG_LEN
#define BOOTLOADER_DRAM_SEG_START \
	(BOOTLOADER_IRAM_SEG_START - BOOTLOADER_DRAM_SEG_LEN)

/* Flash */
#ifdef CONFIG_FLASH_SIZE
#define FLASH_SIZE CONFIG_FLASH_SIZE
#else
#define FLASH_SIZE 0x400000
#endif

/* Cached memory */
#define CACHE_ALIGN  CONFIG_MMU_PAGE_SIZE
#define IROM_SEG_ORG 0x42000000
#define IROM_SEG_LEN FLASH_SIZE
#define DROM_SEG_ORG 0x42800000
#define DROM_SEG_LEN FLASH_SIZE
