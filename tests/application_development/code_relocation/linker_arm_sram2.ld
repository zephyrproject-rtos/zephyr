/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Cortex-M platforms.
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/** enable CONFIG_SRAM2 or any other partition in soc Kconfig,
 * this is just an example to show relocation of code/data/bss script
 */
#if defined CONFIG_ARM
  #define CONFIG_SRAM2 1
  #define _SRAM2_DATA_SECTION_NAME  .sram2_data
  #define _SRAM2_BSS_SECTION_NAME   .sram2_bss
  #define _SRAM2_TEXT_SECTION_NAME  .sram2_text

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
  #define SRAM2_ADDR (CONFIG_SRAM_BASE_ADDRESS + RAM_SIZE2)
#else
  #define SRAM2_ADDR (DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) + RAM_SIZE2)
#endif
#endif

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#define RAM_SIZE2 (CONFIG_SRAM_SIZE * 512)
#else
#define RAM_SIZE2 (DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) / 2)
#endif

MEMORY
    {
#ifdef CONFIG_SRAM2
    SRAM2                  (wx) : ORIGIN = SRAM2_ADDR, LENGTH = RAM_SIZE2
#endif
    }

/* Include the SoC-specific linker file */
#include <linker.ld>
