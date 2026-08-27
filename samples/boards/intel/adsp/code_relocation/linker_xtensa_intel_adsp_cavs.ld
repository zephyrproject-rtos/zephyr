/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/* Use SRAM2 for TEXT, SRAM3 for DATA and SRAM4 for BSS.
 * To ensure they are contiguous (and compatible with default SRAM
 * usage), they are laid out like this:
 *
 *  *---------------------------------------------------------------*
 *  |Reserved |    SRAM4    |    SRAM2    |     SRAM0     |  SRAM3  |
 *  *---------------------------------------------------------------*
 *  |         |     BSS     |         TEXT         |     DATA       |
 *  *---------------------------------------------------------------*
 *
 * Note that BSS, TEXT and DATA are contiguous, but that SRAM0 (default
 * RAM) has a bit of both TEXT and DATA. In fact, the default linker
 * script would also place some BSS section on SRAM0, which would break
 * the contiguousness. To avoid that, before including the platform
 * default linker script, all BSS content is put in the SRAM4 region.
 * (More info at the comment before SECTIONS below.)
 *
 * One final detail: SRAM4 actually lives in the "uncached" memory,
 * but `fix_elf_addrs.py` will place it as above for rimage.
 */
#define SRAM2_ADDR  (DT_REG_ADDR(DT_NODELABEL(sram0)) + 32 * 1024)
#define RAM_SIZE2 (CONFIG_HP_SRAM_RESERVE - 32 * 1024)

#define SRAM3_ADDR  (DT_REG_ADDR(DT_NODELABEL(sram3)))
#define RAM_SIZE3  (DT_REG_SIZE(DT_NODELABEL(sram3)))

#define SRAM4_ADDR  ((DT_REG_ADDR(DT_NODELABEL(sram0)) - 512 * 1024 * 1024) + 16 * 1024)
#define RAM_SIZE4  (16 * 1024)

MEMORY
{
  SRAM2                  (wx) : ORIGIN = (SRAM2_ADDR), LENGTH = RAM_SIZE2
  SRAM3                  (wx) : ORIGIN = (SRAM3_ADDR), LENGTH = RAM_SIZE3
  SRAM4                  (wx) : ORIGIN = (SRAM4_ADDR), LENGTH = RAM_SIZE4
}

#define MPU_ALIGN(region_size) \
    . = ALIGN(4)

/* Place all of BSS content in SRAM4. If not done this way, platform default
 * linker script would add them after DATA, thus breaking contiguousness.
 * Note that an empty BSS section will still be generated during build, but
 * is ignored by `fix_elf_addrs.py` when generating artifacts for rimage.
 */
SECTIONS {
  .bss (NOLOAD) :
  {
    . = ALIGN(64);
    _bss_start = .;
    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    *(.sbss2)
    *(.sbss2.*)
    *(.gnu.linkonce.sb2.*)
    *(.dynbss)
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
  #ifdef CONFIG_CODE_DATA_RELOCATION
  #include <linker_sram_bss_relocate.ld>
  #endif
    . = ALIGN(8);
    _bss_end = .;
  } >SRAM4
}

#include <xtensa-cavs-linker.ld>
