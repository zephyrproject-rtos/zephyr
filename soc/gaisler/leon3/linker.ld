/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for LEON3
 */

#include <zephyr/devicetree.h>

#define RAM_ADDR DT_CHOSEN_SRAM_ADDR
#define RAM_SIZE DT_CHOSEN_SRAM_SIZE

MEMORY
{
  rom      (rx)  : ORIGIN = 0x00000000, LENGTH = 512M
  RAM      (rwx) : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE
  /* refer to include/linker/inlist.ld */
  IDT_LIST (wx)  : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}

REGION_ALIAS("REGION_TEXT", RAM);
REGION_ALIAS("REGION_RODATA", RAM);
REGION_ALIAS("REGION_DATA_VMA", RAM);
REGION_ALIAS("REGION_DATA_LMA", RAM);
REGION_ALIAS("REGION_BSS", RAM);

#define ROMABLE_REGION RAM
#define RAMABLE_REGION RAM

#include <zephyr/arch/sparc/linker.ld>
