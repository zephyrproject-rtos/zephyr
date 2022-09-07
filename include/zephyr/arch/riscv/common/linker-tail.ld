/*
 * Copyright (c) 2016-2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generic linker script footer for RISC-V.
 */

#include <zephyr/linker/debug-sections.ld>

    /DISCARD/ : { *(.note.GNU-stack) }

    SECTION_PROLOGUE(.riscv.attributes, 0,)
	{
	KEEP(*(.riscv.attributes))
	KEEP(*(.gnu.attributes))
	}

    /* Sections generated from 'zephyr,memory-region' nodes */
    LINKER_DT_SECTIONS()

/* Because ROMABLE_REGION != RAMABLE_REGION in XIP-system, it is valid
 * to set __rom_region_end symbol at the end of linker script and
 * doesn't mistakenly contain the RAMABLE_REGION in it.
 */
#ifdef CONFIG_XIP
/* Must be last in romable region */
SECTION_PROLOGUE(.last_section,(NOLOAD),)
{
} GROUP_LINK_IN(ROMABLE_REGION)

/* To provide the image size as a const expression,
 * calculate this value here. */
__rom_region_end = LOADADDR(.last_section);
__rom_region_size = __rom_region_end - __rom_region_start;
#endif

}
