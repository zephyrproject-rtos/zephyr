/* linker.ld - Linker command/script file */

/*
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define rom_start .isr_vectors

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>

SECTIONS
{
  /* Standard Zephyr relocation section */
#include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif

#ifdef CONFIG_OPENAMP_RSC_TABLE
  SECTION_PROLOGUE(.resource_table,, SUBALIGN(16))
  {
    KEEP(*(.resource_table*))
  } GROUP_LINK_IN(ROMABLE_REGION)
#endif
}
