/*
 * Copyright (c) 2021, Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/devicetree.h>

MEMORY
     {
#if defined(CONFIG_CODE_DDR)
        DDR  (wx) : ORIGIN = 0x80400000, LENGTH = 0x00C00000
#else
        DDR  (wx) : ORIGIN = 0x80000000, LENGTH = 0x01000000
#endif
     }

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>

SECTIONS
     {
#ifdef CONFIG_OPENAMP_RSC_TABLE
        SECTION_PROLOGUE(.resource_table,, SUBALIGN(8))
        {
            KEEP(*(.resource_table*))
        } GROUP_LINK_IN(ROMABLE_REGION)
#endif
     }
