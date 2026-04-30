/*
 * Copyright 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/** enable CONFIG_SRAM2 or any other partition in soc Kconfig,
 * this is just an example to show relocation of code/data/bss script
 */
#define _SRAM2_DATA_SECTION_NAME  .sram2_data
#define _SRAM2_BSS_SECTION_NAME   .sram2_bss
#define _SRAM2_TEXT_SECTION_NAME  .sram2_text

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#define RAM_SIZE2 (CONFIG_SRAM_SIZE * 512)
#define RAM_BASE2 (CONFIG_SRAM_BASE_ADDRESS + RAM_SIZE2)
#else
#define RAM_SIZE2 (DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) / 2)
#define RAM_BASE2 (DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) + RAM_SIZE2)
#endif

MEMORY
    {
    SRAM2                  (wx) : ORIGIN = RAM_BASE2, LENGTH = RAM_SIZE2
    }

#include <zephyr/arch/riscv/common/linker.ld>
