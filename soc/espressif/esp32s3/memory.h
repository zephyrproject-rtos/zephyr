/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

/* SRAM0 (32k), SRAM1 (416k), SRAM2 (64k) memories
 * Ibus and Dbus address space
 */
#define SRAM0_IRAM_START     DT_REG_ADDR(DT_NODELABEL(sram0))
#define SRAM0_SIZE           DT_REG_SIZE(DT_NODELABEL(sram0))
#define SRAM1_DRAM_START     DT_REG_ADDR(DT_NODELABEL(sram1))
#define SRAM1_IRAM_START     (SRAM0_IRAM_START + SRAM0_SIZE)
#define SRAM_USER_IRAM_START (SRAM0_IRAM_START + CONFIG_ESP32S3_INSTRUCTION_CACHE_SIZE)

#define SRAM2_DRAM_START      DT_REG_ADDR(DT_NODELABEL(sram2))
#define SRAM2_SIZE            DT_REG_SIZE(DT_NODELABEL(sram2))
#define SRAM2_USER_DRAM_START (SRAM2_DRAM_START + CONFIG_ESP32S3_DATA_CACHE_SIZE)
#define SRAM2_USER_DRAM_SIZE  (SRAM2_SIZE - CONFIG_ESP32S3_DATA_CACHE_SIZE)

/** Simplified memory map for the bootloader.
 *  Make sure the bootloader can load into main memory without overwriting itself.
 *
 *  ESP32-S3 ROM static data usage is as follows:
 *  - 0x3fcd7e00 - 0x3fce9704: Shared buffers, used in UART/USB/SPI download mode only
 *  - 0x3fce9710 - 0x3fceb710: PRO CPU stack, can be reclaimed as heap after RTOS startup
 *  - 0x3fceb710 - 0x3fced710: APP CPU stack, can be reclaimed as heap after RTOS startup
 *  - 0x3fced710 - 0x3fcf0000: ROM .bss and .data (not easily reclaimable)
 *
 *  The 2nd stage bootloader can take space up to the end of ROM shared
 *  buffers area (0x3fce9704). For alignment purpose we shall use value (0x3fce9700).
 */

/* The offset between Dbus and Ibus.
 * Used to convert between 0x403xxxxx and 0x3fcxxxxx addresses.
 */
#define IRAM_DRAM_OFFSET         0x6f0000
#define DRAM_BUFFERS_START       0x3fcd7e00
#define DRAM_BUFFERS_END         0x3fce9704
#define DRAM_PROCPU_STACK_START  0x3fce9710
#define DRAM_STACK_START DRAM_PROCPU_STACK_START
#define DRAM_APPCPU_STACK_START  0x3fceb710
#define DRAM_ROM_BSS_DATA_START  0x3fcf0000

/* Set the limit for the application runtime dynamic allocations */
#define DRAM_RESERVED_START      DRAM_BUFFERS_END

/* For safety margin between bootloader data section and startup stacks */
#define BOOTLOADER_STACK_OVERHEAD      0x0
#define BOOTLOADER_DRAM_SEG_LEN        0x15000
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x1a00
#define BOOTLOADER_IRAM_SEG_LEN        0xc000

/* Base address used for calculating memory layout
 * counted from Dbus backwards and back to the Ibus
 */
#define BOOTLOADER_USER_DRAM_END (DRAM_BUFFERS_START - BOOTLOADER_STACK_OVERHEAD)

/* Start of the lower region is determined by region size and the end of the higher region */
#define BOOTLOADER_IRAM_LOADER_SEG_START                                                           \
	(BOOTLOADER_USER_DRAM_END - BOOTLOADER_IRAM_LOADER_SEG_LEN + IRAM_DRAM_OFFSET)
#define BOOTLOADER_IRAM_SEG_START (BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_LEN)
#define BOOTLOADER_DRAM_SEG_END   (BOOTLOADER_IRAM_SEG_START - IRAM_DRAM_OFFSET)
#define BOOTLOADER_DRAM_SEG_START (BOOTLOADER_DRAM_SEG_END - BOOTLOADER_DRAM_SEG_LEN)

/* The "USER_IRAM_END" represents the end of staticaly allocated memory.
 * This address is where 2nd stage bootloader starts allocating memory,
 * and it should not be overlapped by the user image.
 * When there is no 2nd stage bootloader the bootstrapping is done
 * by the so-called SIMPLE_BOOT.
 * NOTE: AMP is supported only if MCUboot is enabled.
 */
#ifdef CONFIG_ESP_SIMPLE_BOOT
#define USER_DRAM_END BOOTLOADER_USER_DRAM_END
#else
#define USER_DRAM_END (BOOTLOADER_IRAM_LOADER_SEG_START - IRAM_DRAM_OFFSET)
#endif
#define USER_IRAM_END (USER_DRAM_END + IRAM_DRAM_OFFSET)

/* AMP */
#if defined(CONFIG_SOC_ENABLE_APPCPU) || defined(CONFIG_SOC_ESP32S3_APPCPU)
#define APPCPU_IRAM_SIZE CONFIG_ESP_APPCPU_IRAM_SIZE
#define APPCPU_DRAM_SIZE CONFIG_ESP_APPCPU_DRAM_SIZE
#define AMP_COMM_SIZE DT_REG_SIZE(DT_NODELABEL(ipmmem0)) + DT_REG_SIZE(DT_NODELABEL(shm0)) +       \
		      DT_REG_SIZE(DT_NODELABEL(ipm0)) + DT_REG_SIZE(DT_NODELABEL(mbox0))
#undef DRAM_RESERVED_START
#define DRAM_RESERVED_START 0x3fce5000
#else
#define APPCPU_IRAM_SIZE 0
#define APPCPU_DRAM_SIZE 0
#define AMP_COMM_SIZE    0
#endif

#define APPCPU_SRAM_SIZE (APPCPU_IRAM_SIZE + APPCPU_DRAM_SIZE)

/* Flash */
#ifdef CONFIG_FLASH_SIZE
#define FLASH_SIZE         CONFIG_FLASH_SIZE
#else
#define FLASH_SIZE         0x800000
#endif

/* Cached memory */
#define CACHE_ALIGN        CONFIG_MMU_PAGE_SIZE
#define IROM_SEG_ORG       0x42000000
#define IROM_SEG_LEN       FLASH_SIZE
#define DROM_SEG_ORG       0x3c000000
#define DROM_SEG_LEN       FLASH_SIZE
