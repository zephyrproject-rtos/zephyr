/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

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

/* RTC SLOW RAM (8k) */
#define RCT_SLOW_RAM_START DT_REG_ADDR(DT_NODELABEL(rtc_slow_ram))
#define RCT_SLOW_RAM_SIZE  DT_REG_SIZE(DT_NODELABEL(rtc_slow_ram))

/* RTC FAST RAM (8k) */
#define RCT_FAST_RAM_START DT_REG_ADDR(DT_NODELABEL(rtc_fast_ram))
#define RCT_FAST_RAM_SIZE  DT_REG_SIZE(DT_NODELABEL(rtc_fast_ram))

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
#define DRAM_SHARED_BUFFERS_START       0x3fcd7e00
#define DRAM_SHARED_BUFFERS_END         0x3fce9704
#define DRAM_PROCPU_STACK_START  0x3fce9710
#define DRAM_STACK_START DRAM_PROCPU_STACK_START
#define DRAM_APPCPU_STACK_START  0x3fceb710
#define DRAM_ROM_BSS_DATA_START  0x3fcf0000

/* Upper boundary of user-usable SRAM */
#define DRAM_USER_END      DRAM_SHARED_BUFFERS_END

/* Safety margin between MCUboot segments and ROM stack */
#define BOOTLOADER_STACK_OVERHEAD      0x2000
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x1a00

/* Upper limit of SRAM available for MCUboot bootloader segments */
#define BOOTLOADER_USER_DRAM_END \
	((DRAM_SHARED_BUFFERS_END - BOOTLOADER_STACK_OVERHEAD) & ~0xFF)

#define BOOTLOADER_IRAM_LOADER_SEG_START                                                           \
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
	((BOOTLOADER_IRAM_LOADER_SEG_START - IRAM_DRAM_OFFSET - SRAM1_DRAM_START) / 4)
#define BOOTLOADER_IRAM_SEG_START \
	ALIGN_UP(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_TARGET_LEN, 0x400)
#define BOOTLOADER_IRAM_SEG_LEN \
	(BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_START)
#define BOOTLOADER_DRAM_SEG_LEN   BOOTLOADER_IRAM_SEG_LEN
#define BOOTLOADER_DRAM_SEG_START \
	(BOOTLOADER_IRAM_SEG_START - IRAM_DRAM_OFFSET - BOOTLOADER_DRAM_SEG_LEN)

/* Upper boundary for user DRAM allocation */

/* AMP */
#if defined(CONFIG_SOC_ESP32S3_APPCPU)
#if defined(CONFIG_SOC_ENABLE_APPCPU)
#define APPCPU_IRAM_SIZE CONFIG_ESP_APPCPU_IRAM_SIZE
#define APPCPU_DRAM_SIZE CONFIG_ESP_APPCPU_DRAM_SIZE
#define AMP_COMM_SIZE DT_REG_SIZE(DT_NODELABEL(ipmmem0)) + DT_REG_SIZE(DT_NODELABEL(shm0)) +       \
		      DT_REG_SIZE(DT_NODELABEL(ipm0)) + DT_REG_SIZE(DT_NODELABEL(mbox0))
#undef DRAM_USER_END
#define DRAM_USER_END DT_REG_ADDR(DT_NODELABEL(ipmmem0))
#define APPCPU_IROM_SIZE CONFIG_ESP_APPCPU_IROM_SIZE
#define APPCPU_DROM_SIZE CONFIG_ESP_APPCPU_DROM_SIZE
#else
/* Fallback for non-AMP APPCPU builds */
#define APPCPU_IRAM_SIZE 0x8000
#define APPCPU_DRAM_SIZE 0x8000
#define APPCPU_IROM_SIZE 0x10000
#define APPCPU_DROM_SIZE 0x10000
#endif /* CONFIG_SOC_ENABLE_APPCPU */
#else
#define APPCPU_IRAM_SIZE 0
#define APPCPU_DRAM_SIZE 0
#define AMP_COMM_SIZE    0
#define APPCPU_IROM_SIZE 0
#define APPCPU_DROM_SIZE 0
#endif

#define APPCPU_SRAM_SIZE (APPCPU_IRAM_SIZE + APPCPU_DRAM_SIZE)
#define APPCPU_ROM_SIZE (APPCPU_IROM_SIZE + APPCPU_DROM_SIZE)

/* Upper boundary for user DRAM allocation.
 * For AMP builds, capped at IPC shared memory start.
 */
#ifdef CONFIG_ESP_SIMPLE_BOOT
#define USER_DRAM_END DRAM_SHARED_BUFFERS_END
#elif defined(CONFIG_SOC_ENABLE_APPCPU)
#define USER_DRAM_END DRAM_USER_END
#else
#define USER_DRAM_END (BOOTLOADER_IRAM_LOADER_SEG_START - IRAM_DRAM_OFFSET)
#endif
#define USER_IRAM_END (USER_DRAM_END + IRAM_DRAM_OFFSET)

/* Cached memory */
#define ICACHE0_START DT_REG_ADDR(DT_NODELABEL(icache0))
#define ICACHE0_SIZE  DT_REG_SIZE(DT_NODELABEL(icache0))
#define DCACHE0_START DT_REG_ADDR(DT_NODELABEL(dcache0))
#define DCACHE0_SIZE  DT_REG_SIZE(DT_NODELABEL(dcache0))

#define CACHE_ALIGN   CONFIG_MMU_PAGE_SIZE

/* Flash memory */
#ifdef CONFIG_FLASH_SIZE
#define FLASH_SIZE         CONFIG_FLASH_SIZE
#else
#define FLASH_SIZE         0x800000
#endif
