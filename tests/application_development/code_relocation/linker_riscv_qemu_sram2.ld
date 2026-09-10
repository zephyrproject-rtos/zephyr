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

#define RAM_SIZE2 (DT_CHOSEN_SRAM_SIZE / 2)
#define RAM_BASE2 (DT_CHOSEN_SRAM_ADDR + RAM_SIZE2)

MEMORY
    {
    SRAM2                  (wx) : ORIGIN = RAM_BASE2, LENGTH = RAM_SIZE2
    }

#include <zephyr/arch/riscv/common/linker.ld>
