/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for GR716A
 */

/*
 * GR716A memory map
 *
 * LENGTH values represent the maximum possible. All memory regions may not be
 * available on all systems. bootprom, RAM and SRAM are always available.
 */
MEMORY
{
  bootprom (rx)  : ORIGIN = 0x00000000, LENGTH = 4K
  extprom  (rx)  : ORIGIN = 0x01000000, LENGTH = 16M
  spi0     (rx)  : ORIGIN = 0x02000000, LENGTH = 32M
  spi1     (rx)  : ORIGIN = 0x04000000, LENGTH = 32M
  RAM      (rw)  : ORIGIN = 0x30000000, LENGTH = 64K
  SRAM     (x)   : ORIGIN = 0x31000000, LENGTH = 128K
  extram   (rwx) : ORIGIN = 0x40000000, LENGTH = 256M
  /* refer to include/linker/inlist.ld */
  IDT_LIST (wx)  : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}

REGION_ALIAS("REGION_TEXT", SRAM);
REGION_ALIAS("REGION_RODATA", RAM);
REGION_ALIAS("REGION_DATA_VMA", RAM);
REGION_ALIAS("REGION_DATA_LMA", RAM);
REGION_ALIAS("REGION_BSS", RAM);

#define ROMABLE_REGION RAM
#define RAMABLE_REGION RAM

#include <zephyr/arch/sparc/linker.ld>
