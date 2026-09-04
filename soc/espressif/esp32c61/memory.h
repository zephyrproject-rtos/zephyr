/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

/* HP-SRAM (320kB) memory
 * From HAL soc.h: SOC_IRAM_LOW = 0x40800000, SOC_IRAM_HIGH = 0x40850000
 */
#define HPSRAM_START      DT_REG_ADDR(DT_NODELABEL(sramhp))
#define HPSRAM_SIZE       DT_REG_SIZE(DT_NODELABEL(sramhp))
#define HPSRAM_DRAM_START HPSRAM_START
#define HPSRAM_IRAM_START HPSRAM_START

/** Simplified memory map for the bootloader.
 *  Make sure the bootloader can load into main memory without overwriting itself.
 *
 *  ESP32-C61 ROM static data usage is as follows:
 *  - up to 0x4083e670: kept free for the 2nd stage bootloader
 *    iram_loader_seg
 *  - 0x4083e670 - 0x4084c670: ROM shared buffers, used in download
 *    mode only (reclaimable as heap after startup)
 *  - 0x4084c670 - 0x4084e670: ROM CPU stack
 *  - 0x4084e670 - 0x40850000: ROM .bss/.data (not reclaimable)
 *
 *  The 2nd stage bootloader can take space up to the start of the ROM
 *  shared buffers area.
 */

#define DRAM_SHARED_BUFFERS_START 0x4083e670
#define DRAM_SHARED_BUFFERS_END   0x4084c670
#define DRAM_STACK_START          DRAM_SHARED_BUFFERS_END

/* Upper boundary of user-usable SRAM: the ROM shared buffers are
 * reclaimed as heap once the application is running, while the ROM CPU
 * stack above stays reserved.
 */
#define DRAM_USER_END DRAM_SHARED_BUFFERS_END

/* Stack headroom kept free above the loader segments */
#define BOOTLOADER_STACK_OVERHEAD 0x2000

#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x1C00
#define BOOTLOADER_DRAM_LOADER_SEG_LEN 0x0C00

/* Upper limit of SRAM available for MCUboot bootloader segments. The
 * ROM shared buffers end (0x4084c670) is the ceiling the ROM allows for
 * the 2nd stage bootloader; the overhead below it keeps room for the
 * bootloader stack.
 */
#define BOOTLOADER_USER_DRAM_END (DRAM_SHARED_BUFFERS_END - BOOTLOADER_STACK_OVERHEAD)

#define BOOTLOADER_IRAM_LOADER_SEG_START (BOOTLOADER_USER_DRAM_END - BOOTLOADER_IRAM_LOADER_SEG_LEN)
#define BOOTLOADER_DRAM_LOADER_SEG_START                                                           \
	(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_DRAM_LOADER_SEG_LEN)

/* MCUboot iram/dram segments: placed in upper half of SRAM, below iram_loader_seg.
 * On unified-address SoCs these are the same physical memory.
 * The lower half is reserved for the application image.
 */
#define BOOTLOADER_IRAM_SEG_TARGET_LEN ((BOOTLOADER_DRAM_LOADER_SEG_START - HPSRAM_START) / 4)
#define BOOTLOADER_IRAM_SEG_START                                                                  \
	ALIGN_UP(BOOTLOADER_DRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_TARGET_LEN, 0x100)
#define BOOTLOADER_IRAM_SEG_LEN   (BOOTLOADER_DRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_START)
#define BOOTLOADER_DRAM_SEG_LEN   BOOTLOADER_IRAM_SEG_LEN
#define BOOTLOADER_DRAM_SEG_START (BOOTLOADER_IRAM_SEG_START - BOOTLOADER_DRAM_SEG_LEN)

/* Flash */
#define FLASH_SIZE         DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
#define FLASH_BASE_ADDRESS DT_REG_ADDR(DT_CHOSEN(zephyr_flash))

/* Cached memory - ESP32-C61 uses unified I/D address space
 * From HAL ext_mem_defs.h: SOC_IRAM0_CACHE_ADDRESS_LOW = 0x42000000
 */
#define CACHE_ALIGN  CONFIG_MMU_PAGE_SIZE
#define IROM_SEG_ORG 0x42000000
#define IROM_SEG_LEN FLASH_SIZE
/* DROM shares the unified-cache linear address space with IROM. Placing
 * drom0_0_seg at the same origin lets the linker emit .flash.rodata
 * immediately after .text, so the MMU allocator's linear free_head
 * advance (irom_len + drom_len) matches the actual reserved virtual
 * range. This avoids a gap that PSRAM mapping could overrun.
 */
#define DROM_SEG_ORG IROM_SEG_ORG
#define DROM_SEG_LEN FLASH_SIZE

/* External RAM (PSRAM) cache window. Derived from the DT ext_ram node so
 * the linker uses the full virtual range the MMU can map, not the physical
 * chip size. Shares the same bus as IROM/DROM (unified cache). Physical
 * PSRAM size is enforced by the ext_ram-overflow ASSERT in default.ld
 * using CONFIG_ESP_SPIRAM_SIZE.
 */
#define EXTRAM_START DT_REG_ADDR(DT_NODELABEL(ext_ram))
#define EXTRAM_SIZE  DT_REG_SIZE(DT_NODELABEL(ext_ram))
